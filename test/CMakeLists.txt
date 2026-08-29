cmake_minimum_required(VERSION 3.13)
project(unit_tests C)

set(CMAKE_C_STANDARD 23)

add_executable(test_shared
        shared/test_report_descriptor.c
        ${CMAKE_SOURCE_DIR}/../src/shared/report_descriptor.c
        ${CMAKE_SOURCE_DIR}/unity/src/unity.c
)
target_include_directories(test_shared PRIVATE
        ${CMAKE_SOURCE_DIR}/../src/shared
        ${CMAKE_SOURCE_DIR}/unity/src
)

enable_testing()
add_test(NAME shared_tests COMMAND test_shared)
