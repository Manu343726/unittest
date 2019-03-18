#include <unittest/detail/testrunner.hpp>
#include "example.hpp"
#include "example.hpp.tinyrefl"

int main()
{
    return unittest::detail::runTestCase<testing::ExampleTestCase>();
}
