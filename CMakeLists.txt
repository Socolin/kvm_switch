cmake_minimum_required(VERSION 3.13)

# Pull in the SDK — must come before project()
include(pico_sdk_import.cmake)

project(kvm C CXX ASM)

# Set up the SDK
pico_sdk_init()

add_executable(kvm
        kvm.c
        ch9350l.c
        ch9350l.h
        ch9329.c
        ch9329.h
        debug.c
)

pico_enable_stdio_usb(kvm 1)
pico_enable_stdio_uart(kvm 0)

# Link the standard library (gpio, timers, sleep_ms, etc.)
target_link_libraries(kvm pico_stdlib pico_multicore)

# Produce the .uf2 (plus .elf, .bin, map file)
pico_add_extra_outputs(kvm)
