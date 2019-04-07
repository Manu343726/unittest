add_library(libexample SHARED example.cpp)
target_include_directories(libexample PUBLIC "include/")

find_package(tinyrefl_tool REQUIRED)
tinyrefl_tool(TARGET libexample HEADERS include/libexample/example.hpp)
