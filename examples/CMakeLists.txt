add_subdirectory(libexample)

add_unittest(
NAME
  test_example
TESTS
  test_example.hpp
DEPENDENCIES
  libexample
)
