#include <unittest/unittest.hpp>
#include <libexample/example.hpp>
#include <libexample/example.hpp.tinyrefl>

namespace testing
{

struct ExampleTestCase : public unittest::TestCase
{
    [[unittest::patch("mynamespace::ExampleClass::identity(int) const")]]
    void test_another_one_bites_the_dust(unittest::MethodSpy<int(int)>& identity)
    {
        mynamespace::ExampleClass object;

        self.assertEqual(object.methodThatCallsIdentity(), 42);
        identity.assert_called_once_with(42);
    }
};

}
