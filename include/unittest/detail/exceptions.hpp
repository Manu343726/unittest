#ifndef UNITTEST_DETAIL_EXCEPTIONS_HPP
#define UNITTEST_DETAIL_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>
#include <backward/backward.hpp>

namespace unittest
{

namespace detail
{

class TestAssertionException final : public std::exception
{
public:
    TestAssertionException(std::string what, backward::StackTrace backtrace) :
        _what{std::move(what)},
        backtrace{std::move(backtrace)}
    {}

    const char* what() const noexcept override
    {
        return _what.c_str();
    }

    backward::StackTrace backtrace;

private:
    std::string _what;
};

} // namespace unittest::detail

} // namespace unittest

#endif // UNITTEST_DETAIL_EXCEPTIONS_HPP
