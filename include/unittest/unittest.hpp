#ifndef UNITTEST_UNITTEST_HPP
#define UNITTEST_UNITTEST_HPP

#include <unittest/detail/exceptions.hpp>

namespace unittest
{

class TestCase
{
public:
    template<typename T>
    void assertTrue(const T& value)
    {
        assertImpl(value, static_cast<bool>(value), true);
    }

private:
    template<typename T, typename U>
    void assertImpl(const T& value, const bool result, const U& expected)
    {
        if(!result)
        {
            throw unittest::detail::TestAssertionException{fmt::format(
                    "Assertion failure: Expected value {} to equal {}", value, expected)};
        }
    }
};

} // namespace unittest

#endif // UNITTEST_UNITTEST_HPP
