set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS_DEBUG "-O0")

add_subdirectory(libexample)

add_executable(test_example main.cpp)
target_include_directories(test_example PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(test_example PRIVATE libexample unittest)

tinyrefl_tool(TARGET test_example HEADERS
    test_example.hpp)
