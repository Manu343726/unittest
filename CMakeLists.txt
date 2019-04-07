cmake_minimum_required(VERSION 3.0)
project(unittest)

if(CMAKE_CURRENT_SOURCE_DIR EQUAL CMAKE_SOURCE_DIR)
    set(UNITTEST_IS_MAIN_PROJECT TRUE)
else()
    set(UNITTEST_IS_MAIN_PROJECT TRUE)
endif()

set(CONANBUILDINFO_FILE "${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake")

if(NOT EXISTS "${CONANBUILDINFO_FILE}")
    message(FATAL_ERROR "conanbuildinfo.cmake file (${CONANBUILDINFO_FILE}) not found. Please run conan install ${CMAKE_CURRENT_SOURCE_DIR} --build=missing from your build directory first")
else()
    include("${CONANBUILDINFO_FILE}")
    conan_basic_setup(TARGETS)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(UNITTEST_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(unittest INTERFACE)
target_include_directories(unittest INTERFACE "${UNITTEST_INCLUDE_DIR}")
target_link_libraries(unittest INTERFACE
    CONAN_PKG::tinyrefl
    CONAN_PKG::ctti
    CONAN_PKG::CTRE
    CONAN_PKG::fmt
    CONAN_PKG::backward
    CONAN_PKG::elfspy
    dw
)
target_compile_definitions(unittest INTERFACE
    BACKWARD_HAS_UNWIND=1
    BACKWARD_HAS_BACKTRACE=0
    BACKWARD_HAS_BACKTRACE_SYMBOL=0
    BACKWARD_HAS_DW=1
    BACKWARD_HAS_BFD=0
)

option(UNITTESTS_BUILD_EXAMPLES "Build unittest examples" ${UNITTEST_IS_MAIN_PROJECT})

if(UNITTESTS_BUILD_EXAMPLES)
    include(unittest.cmake)
    add_subdirectory(examples)
endif()
