cmake_minimum_required(VERSION 3.13)
project(unit_tests C)

set(CMAKE_C_STANDARD 23)

add_executable(test_shared
        shared/test_hid_report_descriptor.c
        ${CMAKE_SOURCE_DIR}/../src/shared/hid_report_descriptor.c
        ${CMAKE_SOURCE_DIR}/unity/src/unity.c
        test_utils/test_utils_string.c
        test_utils/string_diff.c
        test_utils/test_utils_file.c
)
target_compile_definitions(test_shared PRIVATE
        TEST_SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(test_shared PRIVATE
        ${CMAKE_SOURCE_DIR}/../src/shared
        ${CMAKE_SOURCE_DIR}/unity/src
)

enable_testing()
add_test(NAME shared_tests COMMAND test_shared)
