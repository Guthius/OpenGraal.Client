cmake_minimum_required(VERSION 4.1)

project(
        OpenGraal
        VERSION 0.1.0
        LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(OPENGRAAL_BUILD_TESTS "Build the test suite" ON)

add_subdirectory(vendor)
add_subdirectory(src)
add_subdirectory(tools)

if (OPENGRAAL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
