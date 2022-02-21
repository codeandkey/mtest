cmake_minimum_required (VERSION 2.8.12)

if (NOT MTEST_SOURCE_DIR)
    set(MTEST_SOURCE_DIR ${CMAKE_SOURCE_DIR})
endif()

file(GLOB_RECURSE MTEST_SOURCES "${CMAKE_SOURCE_DIR}" "*.cpp")

foreach(source ${MTEST_SOURCES})
    string(REGEX REPLACE ".*CMakeFiles.*" "" testsource "${source}")

    if (testsource)
        list(APPEND testsources "${testsource}")
    endif()
endforeach()

foreach(testsource ${testsources})
    file(STRINGS ${testsource} testlines REGEX "^[ \n\t\r]*TEST[ \n\t\r]*\\([^\\)]+\\).*")

    foreach (testline ${testlines})
        string (REGEX REPLACE "\\).*" "" testname "${testline}")
        string (REGEX REPLACE ".*\\(" "" testname "${testname}")
        message("> Discovered test : ${testname}")
        add_test(NAME "${testname}" COMMAND ctest_example "${testname}")
    endforeach()
endforeach()

foreach(testname ${MTEST_TESTS})
endforeach()
