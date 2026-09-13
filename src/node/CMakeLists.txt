add_compile_options(-Wall -Wextra)
add_executable(kvm_node
        # can use 'tinyusb_pico_pio_usb' library later when pico-sdk is updated
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c

        main.c
        kvm_switch_node.c
        node_link_node.c
        ../shared_usb/usb_device.c
)

pico_enable_stdio_uart(kvm_node 1)

add_dependencies(kvm_node GetGitVersion)
target_include_directories(kvm_node PRIVATE ${CMAKE_BINARY_DIR}/generated)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(kvm_node PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(kvm_node PRIVATE PIO_USB_USE_TINYUSB)
target_compile_definitions(kvm_node PRIVATE KVM_NODE)

# Link the standard library (gpio, timers, sleep_ms, etc.)
target_link_libraries(kvm_node
        shared
        pico_stdlib
        pico_multicore
        hardware_spi
        tinyusb_device
        tinyusb_board)

target_link_options(kvm_node PUBLIC -Wl,--print-memory-usage)

# Produce the .uf2 (plus .elf, .bin, map file)
pico_add_extra_outputs(kvm_node)
