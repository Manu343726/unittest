#ifndef UNITTEST_END_MAIN_HPP_INCLUDED
#define UNITTEST_END_MAIN_HPP_INCLUDED

#include <tinyrefl/entities.hpp>

using test_cases = tinyrefl::meta::filter_t<
    tinyrefl::meta::defer<unittest::detail::is_testcase_metatype>,
    tinyrefl::entities
>;

int main(int argc, char** argv)
{
    spy::initialise(argc, argv);
    return unittest::detail::runTestCases<test_cases>();
}

#endif // UNITTEST_END_MAIN_HPP_INCLUDED
