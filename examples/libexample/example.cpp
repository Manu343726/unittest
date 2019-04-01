#include <libexample/example.hpp>

using namespace mynamespace;

int ExampleClass::identity(int input) const
{
    return input;
}

int ExampleClass::methodThatCallsIdentity() const
{
    return identity(42);
}
