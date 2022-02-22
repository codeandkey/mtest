cmake_minimum_required(VERSION 3.22)

project(CTestExample)

set(CMAKE_BUILD_TYPE Debug)

set (SOURCES shared.cpp)
set (TEST_SOURCES tests.cpp test_main.cpp ../../mtest.cpp)
set (APP_SOURCES main.cpp)

add_executable(ctest_example_main ${SOURCES} ${APP_SOURCES})

add_executable(ctest_example_test ${SOURCES} ${TEST_SOURCES})
target_link_libraries(ctest_example_test pthread)

enable_testing()
set(MTEST_RUNNER ctest_example_test)
include(mtest.cmake)
