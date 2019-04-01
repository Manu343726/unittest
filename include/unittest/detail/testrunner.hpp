#ifndef UNITTEST_DETAIL_TESTRUNNER_HPP
#define UNITTEST_DETAIL_TESTRUNNER_HPP

#include <ctti/detail/cstring.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <ctre.hpp>
#include <tinyrefl/api.hpp>
#include <unittest/detail/exceptions.hpp>
#include <unittest/unittest.hpp>
#include <chrono>
#include <optional>
#include <elfspy/SPY.h>
#include <elfspy/Call.h>
#include <elfspy/Result.h>
#include <elfspy/Arg.h>

namespace unittest
{

namespace detail
{

template<typename Method>
constexpr bool is_test_method()
{
    constexpr std::size_t prefix_length = sizeof("test_") - 1;
    return Method::name.name().size() >= prefix_length and Method::name.name()(0, prefix_length) == "test_";
}

struct AssertionFailure
{
    std::string methodName;
    std::string testCaseName;
    std::string message;
    std::optional<backward::StackTrace> backtrace;
};

int print_failures(const std::vector<AssertionFailure>& failures, const int totalTestsRun, const std::chrono::milliseconds& elapsedTime)
{
    for(const auto& failure : failures)
    {
        fmt::print(stderr, "\n====================================================\n");
        fmt::print(stderr, "FAIL: {} ({})\n", failure.methodName, failure.testCaseName);
        fmt::print(stderr, "----------------------------------------------------\n");

        if(failure.backtrace)
        {
            backward::Printer backtracePrinter;
            backtracePrinter.print(failure.backtrace.value(), stderr);
        }

        fmt::print(stderr, "\n{}\n", failure.message);
    }

    fmt::print("\n----------------------------------------------------\n");
    fmt::print("Ran {} tests in {: .3f}s\n", totalTestsRun, elapsedTime.count() / 1000.0f);

    if(failures.empty())
    {
        fmt::print("\nOK\n");
        return EXIT_SUCCESS;
    }
    else
    {
        fmt::print("\nFAILED (failures={})\n", failures.size());
        return EXIT_FAILURE;
    }
}

template<typename Method>
void dump_method_decorators(Method method)
{
    for(const auto& attribute : method.get_attributes())
    {
        if(attribute.namespace_.full_name() == "unittest")
        {
            fmt::print("{} has unittest decorator {}\n", Method::name.full_name(), attribute.full_attribute);
        }
    }
}

template<typename MetaType, tinyrefl::entity Kind = MetaType::kind>
struct is_testcase_metatype_impl : tinyrefl::meta::false_ {};

template<typename MetaType>
struct is_testcase_metatype_impl<MetaType, tinyrefl::entity::CLASS> : std::is_base_of<unittest::TestCase, typename MetaType::class_type>
{};

template<typename MetaType>
struct is_testcase_metatype : is_testcase_metatype_impl<MetaType> {};

template<typename Type>
struct MethodSignature;

template<typename R, typename Class, typename... Args>
struct MethodSignature<R(Class::*)(Args...)>
{
    using return_type = R;
    using class_type = Class;
    using args = tinyrefl::meta::list<Args...>;
    static constexpr bool is_const = false;
};

template<typename R, typename Class, typename... Args>
struct MethodSignature<R(Class::*)(Args...) const>
{
    using return_type = R;
    using class_type = Class;
    using args = tinyrefl::meta::list<Args...>;
    static constexpr bool is_const = true;
};

template<typename MethodMetadata>
using method_return_type = typename MethodSignature<typename MethodMetadata::pointer_type>::return_type;
template<typename MethodMetadata>
using method_class_type = typename MethodSignature<typename MethodMetadata::pointer_type>::class_type;
template<typename MethodMetadata>
using method_args_types = typename MethodSignature<typename MethodMetadata::pointer_type>::args;

template<typename MethodMetadata, typename R, typename Class, typename Args>
struct MethodSpyImpl;

template<typename MethodMetadata, typename R, typename Class, typename... Args>
struct MethodSpyImpl<MethodMetadata, R, Class, tinyrefl::meta::list<Args...>>
    : public spy::Hook<MethodSpyImpl<MethodMetadata, R, Class, tinyrefl::meta::list<Args...>>, R, Class*, Args...>,
      public MethodSpy<R(Args...)>
{
    using This = MethodSpyImpl<MethodMetadata, R, Class, tinyrefl::meta::list<Args...>>;
    using Base = spy::Hook<This, R, Class*, Args...>;

    MethodSpyImpl() :
        Base{
            MethodMetadata::full_display_name.begin(),
            spy::Method<R, Class, Args...>{MethodMetadata{}.get()}.resolve()
        }
    {}

    bool assert_called() override final
    {
        return spy::call(*this).count() > 0;
    }

    bool assert_called_once() override final
    {
        return spy::call(*this).count() == 1;
    }

    bool assert_called_once_with(Args... args) override final
    {
        return assert_called_once() && call_has_args(0, args...);
    }

    bool assert_called_with(Args... args) override final
    {
        for(std::size_t i = 0; i < spy::call(*this).count(); ++i)
        {
            if(call_has_args(i, args...))
            {
                return true;
            }
        }

        return false;
    }

    using ArgsTuple = std::tuple<std::decay_t<Args>...>;
    using ArgsTuples = std::vector<ArgsTuple>;

    ArgsTuples call_args_list() override final
    {
        ArgsTuples result;

        for(std::size_t i = 0; i < spy::call(*this).count(); ++i)
        {
            result.emplace_back(call_args(i));
        }

        return result;
    }

private:
    bool call_has_args(const std::size_t call_index, Args... args)
    {
        return call_args(call_index) == std::make_tuple(args...);
    }

    ArgsTuple call_args(const std::size_t call_index)
    {
        return call_args_impl(std::make_index_sequence<sizeof...(Args)>{}, call_index);
    }

    template<std::size_t... Is>
    ArgsTuple call_args_impl(std::index_sequence<Is...>, const std::size_t call_index)
    {
        return std::make_tuple(spy::arg<Is + 1 /* + 1 to ignore object (first arg) */>(*this).value(call_index)...);
    }
};



template<typename MethodMetadata>
struct MethodSpyInstance : public MethodSpyImpl<
    MethodMetadata,
    method_return_type<MethodMetadata>,
    method_class_type<MethodMetadata>,
    method_args_types<MethodMetadata>
> {};

template<typename TestCase>
void runTestCase(int& total_tests, std::vector<AssertionFailure>& failures)
{
    static_assert(std::is_base_of_v<unittest::TestCase, TestCase> &&
                  tinyrefl::has_metadata<TestCase>(),
        "Expected test case class, that is, a class inheriting from unittest::TestCase"
        " and static reflection metadata");

    TestCase testCase;

    tinyrefl::visit_class<TestCase>([&](auto /* testName */, auto /* depth */, auto method,
        TINYREFL_STATIC_VALUE(tinyrefl::entity::MEMBER_FUNCTION)) -> std::enable_if_t<
            is_test_method<decltype(method)>()>
    {
        using Method = decltype(method);
        constexpr Method constexpr_method;

        fmt::print("{} ({}) ... ", Method::name.name(), tinyrefl::metadata<TestCase>::name.full_name());

        try
        {
            if constexpr (constexpr_method.has_attribute("patch") &&
                          constexpr_method.get_attribute("patch").namespace_.full_name() == "unittest" &&
                          constexpr_method.get_attribute("patch").args.size() >= 1)
            {
                constexpr auto target_id = constexpr_method.get_attribute("patch").args[0].pad(1,1);

                if constexpr (tinyrefl::has_entity_metadata<target_id.hash()>())
                {
                    using Target = tinyrefl::entity_metadata<target_id.hash()>;
                    fmt::print("Patching {} in test {}\n", target_id, Method::name.full_name());

                    MethodSpyInstance<Target> spy;
                    method.get(testCase, spy);
                }
                else
                {
                    static_assert(sizeof(TestCase) != sizeof(TestCase),
                        "[[unittest::patch()]] target not found");
                }
            }
            else
            {
                method.get(testCase);
            }
            fmt::print("ok\n");
        }catch(const unittest::detail::TestAssertionException& ex)
        {
            fmt::print("FAIL\n");

            failures.push_back({
                Method::name.name().str(),
                tinyrefl::metadata<TestCase>::name.full_name().str(),
                ex.what(),
                std::make_optional<backward::StackTrace>(ex.backtrace)
            });

        }catch(const std::exception& ex)
        {
            fmt::print("FAIL\n");

            failures.push_back({
                Method::name.name().str(),
                tinyrefl::metadata<TestCase>::name.full_name().str(),
                fmt::format("Unhandled exception thrown while running test case: {}", ex.what()),
                std::nullopt
            });
        }

        total_tests++;
    });
}

template<typename TestCases>
int runTestCases()
{
    int total_tests = 0;
    std::vector<AssertionFailure> failures;
    const auto start = std::chrono::high_resolution_clock::now();

    tinyrefl::meta::foreach<TestCases>([&](auto type, auto /* index */)
    {
        using TestCaseMetadata = typename decltype(type)::type;

        runTestCase<typename TestCaseMetadata::class_type>(total_tests, failures);
    });

    return print_failures(failures, total_tests,
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start));
}

} // namespace unittest::detail

} // namespace unittest

#endif // UNITTEST_DETAIL_TESTRUNNER_HPP
