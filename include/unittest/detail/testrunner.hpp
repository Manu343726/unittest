#ifndef UNITTEST_DETAIL_TESTRUNNER_HPP
#define UNITTEST_DETAIL_TESTRUNNER_HPP

#include <ctti/detail/cstring.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <ctre.hpp>
#include <tinyrefl/api.hpp>
#include <unittest/detail/exceptions.hpp>
#include <unittest/unittest.hpp>

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

template<typename TestCase>
bool runTestCase()
{
    static_assert(std::is_base_of_v<unittest::TestCase, TestCase> &&
                  tinyrefl::has_metadata<TestCase>(),
        "Expected test case class, that is, a class inheriting from unittest::TestCase"
        " and static reflection metadata");

    TestCase testCase;
    bool noFailures = true;
    int total_tests = 0;

    tinyrefl::visit_class<TestCase>([&](auto /* testName */, auto /* depth */, auto method,
        TINYREFL_STATIC_VALUE(tinyrefl::entity::MEMBER_FUNCTION)) -> std::enable_if_t<
            is_test_method<decltype(method)>()>
    {
        if(noFailures)
        {
            using Method = decltype(method);

            fmt::print("{} ({}) ... ", Method::name.name(), tinyrefl::metadata<TestCase>::name.full_name());

            try
            {
                method.get(testCase);

                fmt::print("OK\n");
            }catch(const unittest::detail::TestAssertionException& ex)
            {
                fmt::print("FAIL\n");
                fmt::print(stderr, "\n====================================================\n");
                fmt::print(stderr, "FAIL: {} ({})\n", Method::name.name(), tinyrefl::metadata<TestCase>::name.full_name());
                fmt::print(stderr, "----------------------------------------------------\n");
                fmt::print(stderr, "\n{}\n", ex.what());

                noFailures = false;
            }catch(const std::exception& ex)
            {
                fmt::print("FAIL\n");
                fmt::print(stderr, "\nUnhandled exception thrown while running test case\n", ex.what());

                noFailures = false;
            }

            total_tests++;
        }
    });



    return noFailures;
}

} // namespace unittest::detail

} // namespace unittest

#endif // UNITTEST_DETAIL_TESTRUNNER_HPP
