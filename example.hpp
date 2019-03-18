#include <unittest/unittest.hpp>

namespace testing
{

struct ExampleTestCase : public unittest::TestCase
{
    void test_example()
    {
        assertTrue(1);
    }

    void not_a_test()
    {

    }

    void test_something()
    {
        assertTrue(1);
    }

    void test_fails()
    {
        assertTrue(nullptr);
    }
};

}
