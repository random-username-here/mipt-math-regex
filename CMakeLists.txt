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
    source/MathRegex/Algorithms/fsaf.cpp
)

add_executable(regex demo/Regex.cpp)
target_link_libraries(regex PRIVATE math-regex)

add_executable(ops demo/Ops.cpp)
target_link_libraries(ops PRIVATE math-regex)

if(TESTS)
    message("Tests are ENABLED")
    enable_testing()
    include(CTest)
    find_package(Catch2 3 REQUIRED)

    add_executable(tests)
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain math-regex)
    target_sources(tests PRIVATE
        test/Utils.cpp
        test/Automata.cpp
        test/Make.cpp
        test/Regex.cpp
        test/Algorithms.cpp
    )
    add_test(NAME base COMMAND tests)

    add_custom_target(run-tests
        COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --output-on-failure
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests"
    )
    add_dependencies(run-tests tests)

    if(CODECOV)
        message("Codecov is ENABLED")
        target_compile_options(math-regex PRIVATE --coverage -DNDEBUG) # no asserts
        target_link_options(math-regex PRIVATE --coverage)
        #target_compile_options(tests PRIVATE --coverage -DNDEBUG)
        #target_link_options(tests PRIVATE --coverage)
        find_program(GCOVR_PATH gcovr REQUIRED)
        add_custom_target(run-coverage
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
            COMMAND ${GCOVR_PATH}
                -r ${CMAKE_SOURCE_DIR}
                --filter ${CMAKE_SOURCE_DIR}/source
                --exclude-unreachable-branches
                --exclude-throw-branches # not used here, but used in svector for programmer errors
                ${CMAKE_BINARY_DIR}
                --html-details
                -o ${CMAKE_BINARY_DIR}/coverage.html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage"
        )
        add_dependencies(run-coverage run-tests)
    endif()
endif()

