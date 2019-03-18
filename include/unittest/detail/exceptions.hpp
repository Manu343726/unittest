#ifndef UNITTEST_DETAIL_EXCEPTIONS_HPP
#define UNITTEST_DETAIL_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

namespace unittest
{

namespace detail
{

class TestAssertionException final : public std::exception
{
public:
    TestAssertionException(std::string what) :
        _what{std::move(what)}
    {}

    const char* what() const noexcept override
    {
        return _what.c_str();
    }

private:
    std::string _what;
};

} // namespace unittest::detail

} // namespace unittest

#endif // UNITTEST_DETAIL_EXCEPTIONS_HPP
