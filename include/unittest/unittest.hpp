#ifndef UNITTEST_UNITTEST_HPP
#define UNITTEST_UNITTEST_HPP

#include <unittest/detail/exceptions.hpp>
#include <elfspy/SPY.h>
#include <tinyrefl/api.hpp>

namespace unittest
{

template<typename Signature>
struct MethodSpy;

template<typename R, typename... Args>
struct MethodSpy<R(Args...)>
{
    virtual ~MethodSpy() = default;

    using CallArgs = std::tuple<Args...>;

    virtual bool assert_called() = 0;
    virtual bool assert_called_once() = 0;
    virtual bool assert_called_once_with(Args... args) = 0;
    virtual bool assert_called_with(Args... args) = 0;
    virtual std::vector<std::tuple<std::decay_t<Args>...>> call_args_list() = 0;
};

class TestCase
{
public:
    TestCase():
        self{*this}
    {}

    template<typename T>
    void assertTrue(const T& value)
    {
        assertImpl(value, static_cast<bool>(value), "{value} is not True");
    }

    template<typename T>
    void assertFalse(const T& value)
    {
        assertImpl(value, !static_cast<bool>(value), "{value} is not False");
    }

    template<typename T>
    void assertIsNull(const T& value)
    {
        assertImpl(value, static_cast<bool>(value == nullptr), "{value} is not null");
    }

    template<typename T>
    void assertIsNotNull(const T& value)
    {
        assertImpl(value, static_cast<bool>(value != nullptr), "{value} is null");
    }

    template<typename T, typename U>
    void assertEqual(const T& value, const U& expected)
    {
        assertImpl(value, static_cast<bool>(value == expected),
            "{value} != {expected}", &expected);
    }

    template<typename T, typename U>
    void assertNotEqual(const T& value, const U& expected)
    {
        assertImpl(value, static_cast<bool>(value == expected),
            "{value} == {expected}", &expected);
    }

    TestCase& self;

private:
    template<typename T, typename U = T>
    void assertImpl(const T& value, const bool result, const std::string& message, const U* expected = nullptr)
    {
        if(!result)
        {
            backward::StackTrace st; st.load_here(5);
            st.skip_n_firsts(4);

            if(expected)
            {
                throw unittest::detail::TestAssertionException{fmt::format(
                        "Assertion error: " + message, fmt::arg("value", value), fmt::arg("expected", *expected)),
                        std::move(st)};
            }
            else
            {
                throw unittest::detail::TestAssertionException{fmt::format(
                        "Assertion error: " + message, fmt::arg("value", value)),
                        std::move(st)};
            }
        }
    }
};

} // namespace unittest

#endif // UNITTEST_UNITTEST_HPP
