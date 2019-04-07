enable_testing()

function(add_unittest)
    cmake_parse_arguments(
        "ARGS"
        ""
        "NAME"
        "TESTS;DEPENDENCIES"
        ${ARGN}
    )

    if(NOT ARGS_TESTS)
        message(FATAL_ERROR "Missing TESTS argument with set of tests headers")
    endif()

    if(NOT ARGS_NAME)
        message(FATAL_ERROR "Missing NAME argument with name of the test executable")
    endif()

    set(include_tests)

    foreach(header ${ARGS_TESTS})
        get_filename_component(header_full_path "${header}" ABSOLUTE)
        set(include_tests "${include_tests}#include \"${header_full_path}\"\n")
        set(include_tests "${include_tests}#include \"${header_full_path}.tinyrefl\"\n")
    endforeach()

    set(main_file "${CMAKE_CURRENT_BINARY_DIR}/main.cpp")

    configure_file("${unittest_SOURCE_DIR}/main.cpp.in" "${main_file}")

    add_executable(${ARGS_NAME} ${ARGS_TESTS} ${main_file})

    if(NOT TARGET unittest)
        message(FATAL_ERROR "unittest library target not found")
    endif()

    target_link_libraries(${ARGS_NAME} PRIVATE unittest)

    if(ARGS_DEPENDENCIES)
        target_link_libraries(${ARGS_NAME} PRIVATE ${ARGS_DEPENDENCIES})
    endif()

    find_package(tinyrefl_tool REQUIRED)
    tinyrefl_tool(TARGET ${ARGS_NAME} HEADERS ${ARGS_TESTS})

    add_test(NAME ${ARGS_NAME} COMMAND ${ARGS_NAME})
endfunction()
