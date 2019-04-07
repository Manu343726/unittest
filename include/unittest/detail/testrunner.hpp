#ifndef UNITTEST_DETAIL_TESTRUNNER_HPP
#define UNITTEST_DETAIL_TESTRUNNER_HPP

#include <ctti/detail/cstring.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
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

FMT_BEGIN_NAMESPACE

namespace internal
{

template<>
struct is_like_std_string<ctti::detail::cstring> : std::true_type {};

}

FMT_END_NAMESPACE

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
    std::size_t longest_separator_length = 0;

    for(const auto& failure : failures)
    {
        const auto failure_title = fmt::format("FAIL: {} ({})\n", failure.methodName, failure.testCaseName);
        const std::string hard_separator(failure_title.size() + 1, '=');
        const std::string soft_separator(failure_title.size() + 1, '-');
        fmt::print(stderr, "\n{}\n", hard_separator);
        fmt::print(stderr, failure_title);
        fmt::print(stderr, "{}\n", soft_separator);

        if(failure.backtrace)
        {
            backward::Printer backtracePrinter;
            backtracePrinter.print(failure.backtrace.value(), stderr);
        }

        fmt::print(stderr, "\n{}\n", failure.message);

        longest_separator_length = std::max(longest_separator_length, soft_separator.size());
    }

    fmt::print("\n{}\n", std::string(longest_separator_length, '-'));
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

    template<std::size_t... Indices>
    MethodSpyImpl(std::index_sequence<Indices...>) :
        Base{
            MethodMetadata::full_display_name.begin(),
            spy::Method<R, Class, Args...>{MethodMetadata{}.get()}.resolve()
        },
        _callsHandle{spy::call(*this)},
        _argHandles{spy::arg<Indices + 1>(*this)...},
        _resultHandle{spy::result(*this)}
    {}

    MethodSpyImpl() :
        MethodSpyImpl{std::index_sequence_for<Args...>{}}
    {}

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

    void assert_called()
    {
        if(!called())
        {
            backward::StackTrace st; st.load_here(4);
            st.skip_n_firsts(3);

            throw unittest::detail::TestAssertionException{
                fmt::format("AssertionError: Expected '{}' to have been called.", MethodMetadata::full_display_name),
                std::move(st)
            };
        }
    }

    void _assert_called_once()
    {
        if(_callsHandle.count() != 1)
        {
            backward::StackTrace st; st.load_here(5);
            st.skip_n_firsts(4);

            throw unittest::detail::TestAssertionException{
                fmt::format("AssertionError: Expected '{}' to have been called once. Called {} times.", MethodMetadata::full_display_name, _callsHandle.count()),
                std::move(st)
            };
        }
    }

    void assert_called_once() override final
    {
        _assert_called_once();
    }

    void assert_called_once_with(Args... args) override final
    {
        _assert_called_once();
        _assert_called_with(args...);
    }

    void _assert_called_with(Args... args)
    {
        if(!called())
        {
            backward::StackTrace st; st.load_here(5);
            st.skip_n_firsts(4);

            throw unittest::detail::TestAssertionException{fmt::format(
                "AssertionError: Expected call: {}\nNot called", dump_call(args...)),
                std::move(st)};
        }
        else
        {
            const auto last_call = last_call_args();
            auto call_args = std::make_tuple(args...);

            if(call_args != last_call)
            {
                backward::StackTrace st; st.load_here(5);
                st.skip_n_firsts(4);

                throw unittest::detail::TestAssertionException{fmt::format(
                    "AssertionError: Expected call: {}\nActual call: {}", dump_call(call_args), dump_call(last_call)),
                    std::move(st)};
            }
        }
    }

    void assert_called_with(Args... args)
    {
        _assert_called_with(args...);
    }

private:
    template<std::size_t Index>
    using ArgHandle = spy::ThunkHandle<typename Base::template ExportN<spy::Arg>::template Type<Index>>;

    template<typename Indices>
    struct ArgHandlesForIndices;

    template<std::size_t... Indices>
    struct ArgHandlesForIndices<std::index_sequence<Indices...>>
    {
        using type = std::tuple<ArgHandle<Indices + 1 /* ignore first (object) param */>...>;
    };

    using ArgHandles = typename ArgHandlesForIndices<std::index_sequence_for<Args...>>::type;
    using ResultHandle = spy::ThunkHandle<spy::Result<typename Base::Result>>;
    using CallsHandle  = spy::ThunkHandle<spy::Call>;

    CallsHandle  _callsHandle;
    ArgHandles   _argHandles;
    ResultHandle _resultHandle;


    void assertFailure(const std::string& message)
    {
    }

    bool called()
    {
        return _callsHandle.count() > 0;
    }

    bool called_once()
    {
        return _callsHandle.count() == 1;
    }

    bool called_once_with(Args... args)
    {
        return assert_called_once() && call_has_args(0, args...);
    }

    bool called_with(Args... args)
    {
        for(std::size_t i = 0; i < _callsHandle.count(); ++i)
        {
            if(call_has_args(i, args...))
            {
                return true;
            }
        }

        return false;
    }
    bool call_has_args(const std::size_t call_index, Args... args)
    {
        return call_args(call_index) == std::make_tuple(args...);
    }

    ArgsTuple call_args(const std::size_t call_index)
    {
        return call_args_impl(std::make_index_sequence<sizeof...(Args)>{}, call_index);
    }

    ArgsTuple last_call_args()
    {
        return call_args(_callsHandle.count() - 1);
    }

    template<std::size_t... Is>
    ArgsTuple call_args_impl(std::index_sequence<Is...>, const std::size_t call_index)
    {
        return std::make_tuple(std::get<Is>(_argHandles).value(call_index)...);
    }

    std::string dump_call_by_index(const std::size_t call_index)
    {
        return dump_call(call_args(call_index));
    }

    std::string dump_call(Args... args)
    {
        return dump_call(std::make_tuple(args...));
    }

    std::string dump_call(const ArgsTuple& args)
    {
        return fmt::format("{}{}", MethodMetadata::name.full_name(), args);
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
