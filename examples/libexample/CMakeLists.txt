add_library(libexample example.cpp)
target_include_directories(libexample PUBLIC "include/")

tinyrefl_tool(TARGET libexample HEADERS include/libexample/example.hpp)
