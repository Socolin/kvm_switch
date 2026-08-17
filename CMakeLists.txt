cmake_minimum_required(VERSION 3.13)

set(target_name kvm)

# Pull in the SDK — must come before project()
include(pico_sdk_import.cmake)

project(kvm C CXX ASM)
set(CMAKE_C_STANDARD 23)

# Set up the SDK
pico_sdk_init()
# 1. Get the current timestamp
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")

# 2. Pass it as a preprocessor definition with escaped quotes
add_compile_definitions(BUILD_DATE="${BUILD_DATE}")

add_subdirectory("./Pico-PIO-USB" pico_pio_usb)

add_executable(${target_name}
        main.c
        ch9350l.c
        ch9350l.h
        ch9329.c
        ch9329.h
        debug.c
        # can use 'tinyusb_pico_pio_usb' library later when pico-sdk is updated
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
        hid_manager.c
        logger.c
        usb_device.c
        kvm_switch.c
        usb_host.c
)

#[[
pico_enable_stdio_usb(${target_name} 1)
]]
pico_enable_stdio_uart(${target_name} 1)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(kvm PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(${target_name} PRIVATE PIO_USB_USE_TINYUSB)

# Link the standard library (gpio, timers, sleep_ms, etc.)
target_link_libraries(${target_name}
        pico_stdlib
        pico_multicore
        pico_pio_usb
        tinyusb_device
        tinyusb_host
        tinyusb_board)

# Produce the .uf2 (plus .elf, .bin, map file)
pico_add_extra_outputs(${target_name})
