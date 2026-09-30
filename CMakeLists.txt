cmake_minimum_required(VERSION 4.0)
project(math-regex)
set(CMAKE_CXX_STANDARD 17)

option(TESTS "Build the tests" OFF)
option(CODECOV "Enable codecov on the library" OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(XXHash REQUIRED IMPORTED_TARGET libxxhash)

add_library(math-regex SHARED)
target_include_directories(math-regex PUBLIC include thirdparty)
target_link_libraries(math-regex PUBLIC PkgConfig::XXHash)
target_sources(math-regex PRIVATE
    source/MathRegex/Utils.cpp
    source/MathRegex/Automata.cpp
    source/MathRegex/Make.cpp
    source/MathRegex/Regex.cpp
    source/MathRegex/Algorithms.cpp
    source/MathRegex/Algorithms/determinize.cpp
    source/MathRegex/Algorithms/minimize.cpp
)

add_executable(demo demo/Main.cpp)
target_link_libraries(demo PRIVATE math-regex)

if(CODECOV)
    target_compile_options(math-regex PRIVATE -coverage)
    target_link_options(math-regex PRIVATE -coverage)
endif()

if(TESTS)
    include(CTest)
    find_package(Catch2 3 REQUIRED)

    add_executable(tests)
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain math-regex)
    target_sources(tests PRIVATE
        test/Utils.cpp
        test/Automata.cpp
    )

    add_test(NAME base COMMAND tests)
endif()

