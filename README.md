# unittest

C++ unit testing and mocking made easy

## What?

`unittest` is a proof of concept C++ unit testing framework inspired by
Python's [`unittest`
package](https://docs.python.org/3/library/unittest.html). 

`unitest` is designed with the following goals in mind:

1. **Zero plumbing code**: No setup functions, no main, no test
   registration. The library uses static reflection to figure out where
   are the tests declared, what is and what's not test case code, etc.

2. **Zero macros**: Every line of user code is just perfectly normal C++
   code.

3. **Non intrusive arbitrary function and class mocking**: No need to use
   virtual function interfaces, mock classes, and dependency injection,
   which couple your library design with the way the mocking framework
   works. `unittest` uses monkey-patching through
   [`elfspy`](https://github.com/Manu343726/elfspy) so mocking is as
   transparent as possible.

4. **Expressive test failure output**: `unittest` not only mimics Python's
   `unittest` API but also its console output, including descriptive
   assertion error details, call arguments, location of the failed
   assertion in the code, etc.


5. **Easy integration**: Just pull the library from [conan](https://conan.io/)
   and use the provided `add_unittest()` CMake function to add a unittest
   executable to your project:

   ``` cmake
    add_unittest(
    NAME
      test_example
    TESTS
      test_example.hpp
    DEPENDENCIES
      libexample
    )
   ```

Here is a full example of a unit test case with `unittest`:

``` cpp
#include <unittest/unittest.hpp>
#include <libexample/example.hpp>
#include <libexample/example.hpp.tinyrefl>

namespace testing
{

struct ExampleTestCase : public unittest::TestCase
{
    [[unittest::patch("mynamespace::ExampleClass::identity(int) const", return_value=42)]]
    void test_another_one_bites_the_dust(unittest::MethodSpy<int(int)>& identity)
    {
        mynamespace::ExampleClass object;

        self.assertEqual(object.methodThatCallsIdentity(), 42);
        identity.assert_called_once_with(43);
    }
};

}
```

``` shell
test_another_one_bites_the_dust (test_example::ExampleTestCase) ... FAIL

=======================================================================
FAIL: test_another_one_bites_the_dust (test_example::ExampleTestCase)
-----------------------------------------------------------------------
Stack trace (most recent call last):
#0    Source "/home/manu343726/Documentos/unittest/examples/test_example.hpp", line 16, in test_another_one_b
ites_the_dust
         13:         mynamespace::ExampleClass object;
         14: 
         15:         self.assertEqual(object.methodThatCallsIdentity(), 42);
      >  16:         identity.assert_called_once_with(43);
         17:     }
         18: };

AssertionError: Expected call: mynamespace::ExampleClass::identity(43)
Actual call: mynamespace::ExampleClass::identity(42)

-----------------------------------------------------------------------
Ran 1 tests in  0.002s

FAILED (failures=1)
```

Who said C++ could not be as expressive as Python?

``` python
import unittest, unittest.mock
import mynamespace

class ExampleTestCase(unittest.TestCase):

    @unittest.mock.patch('mynamespace.ExampleClass.identity', return_value=42)
    def test_another_one_bites_the_dust(self, identity):
        object = mynamespace.ExampleClass()

        self.assertEqual(object.methodThatCallsIdentity(), 42)
        identity.assert_called_once_with(43)
```

``` shell
test_another_one_bites_the_dust (test_example.ExampleTestCase) ... FAIL

======================================================================
FAIL: test_another_one_bites_the_dust (test_example.ExampleTestCase)
----------------------------------------------------------------------
Traceback (most recent call last):
  File "/usr/lib/python3.7/unittest/mock.py", line 1195, in patched
    return func(*args, **keywargs)
  File "/home/manu343726/Documentos/unittest/examples/python_equivalent/test_example.py", line 11, in test_another_one_bites_
the_dust
    identity.assert_called_once_with(43)
  File "/usr/lib/python3.7/unittest/mock.py", line 831, in assert_called_once_with
    return self.assert_called_with(*args, **kwargs)
  File "/usr/lib/python3.7/unittest/mock.py", line 820, in assert_called_with
    raise AssertionError(_error_message()) from cause
AssertionError: Expected call: identity(43)
Actual call: identity(42)

----------------------------------------------------------------------
Ran 1 test in 0.002s

FAILED (failures=1)
```

## Requirements

 - Compiler with C++ 17 support
 - Conan with my [bintray repository](https://bintray.com/beta/#/manu343726/conan-packages?tab=packages) configured.
 - CMake >= 3.0
 - Linux x86_64 (See [elfspy requirements](https://github.com/mollismerx/elfspy/wiki/Dependencies) for details).
 - The code must be built with optimizations disabled and full debug information
   (`-O0 -g3`).
 - libelf, libdwarf, or any other debug info reading library (See [backward-cpp
   install instructions](https://github.com/bombela/backward-cpp#libraries-to-read-the-debug-info) for details).

## Running the examples

``` shell
$ git clone https://github.com/Manu343726/unittest && cd unittest
$ mkdir build && cd build
$ conan install .. --build=missing
$ cmake .. -DCMAKE_BUILD_TYPE=Debug
$ make
$ ctest . -V
```

## Documentation

Yeah, sorry, this is a work in progress PoC. Tthe code is given as ugly as it
looks without any comment or docstring that could make things clear.

## License

Everything is released under MIT license.

## Production ready

Yeah, sure.
