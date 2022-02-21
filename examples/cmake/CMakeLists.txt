cmake_minimum_required(VERSION 3.22)

enable_testing()

project(CTestExample)
include(mtest.cmake)

add_executable(ctest_example ctest_example.cpp tests.cpp ../../mtest.cpp)
