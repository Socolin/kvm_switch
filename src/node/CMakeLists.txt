add_executable(kvm_node
        # can use 'tinyusb_pico_pio_usb' library later when pico-sdk is updated
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c

        ../shared/hid_manager.c
        ../shared/logger.c
        ../shared/usb_device.c

        main.c
        extension_node.c
)

pico_enable_stdio_uart(kvm_node 1)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(kvm_node PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(kvm_node PRIVATE PIO_USB_USE_TINYUSB)

# Link the standard library (gpio, timers, sleep_ms, etc.)
target_link_libraries(kvm_node
        pico_stdlib
        pico_multicore
        tinyusb_device
        tinyusb_board)

# Produce the .uf2 (plus .elf, .bin, map file)
pico_add_extra_outputs(kvm_node)
