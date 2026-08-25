cmake_minimum_required(VERSION 3.13)

# Not tested on pico, only on pico2 but from what I saw, the pico usb does not have enough endpoints
set(PICO_BOARD pico2)
# Pull in the SDK — must come before project()
include(pico_sdk_import.cmake)

project(kvm C CXX ASM)
set(CMAKE_C_STANDARD 23)

# Set up the SDK
pico_sdk_init()

# FIXME: Also add git hash
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")
add_compile_definitions(BUILD_DATE="${BUILD_DATE}")

add_subdirectory("./Pico-PIO-USB" pico_pio_usb)

add_subdirectory(src/shared)
add_subdirectory(src/controller)
add_subdirectory(src/node)
