cmake_minimum_required(VERSION 3.13)

# Not tested on pico, only on pico2 but from what I saw, the pico usb does not have enough endpoints
set(PICO_BOARD pico2)
# Pull in the SDK — must come before project()
include(pico_sdk_import.cmake)

project(kvm C CXX ASM)
set(CMAKE_C_STANDARD 23)

# Set up the SDK
pico_sdk_init()

string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")
add_compile_definitions(BUILD_DATE="${BUILD_DATE}")

# Include Git commit hash
find_package(Git REQUIRED)
set(VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/git_version.h")
add_custom_target(GetGitVersion ALL
        COMMAND ${CMAKE_COMMAND}
        -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/GetGitVersion.cmake
        BYPRODUCTS ${VERSION_HEADER}
)


add_subdirectory("./Pico-PIO-USB" pico_pio_usb)

add_subdirectory(src/shared)
add_subdirectory(src/controller)
add_subdirectory(src/node)
