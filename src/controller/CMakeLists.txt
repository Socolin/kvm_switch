add_compile_options(-Wall -Wextra)
add_executable(kvm_controller
        # can use 'tinyusb_pico_pio_usb' library later when pico-sdk is updated
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c

        main.c
        hid_device_manager.c
        computer_manager.c
        node_link_ctrl.c
        kvm_switch_config.c
        kvm_switch_controller.c
        usb_host.c
        web_usb_handler.c
        ../shared_usb/usb_device.c
)
target_link_options(kvm_controller PUBLIC -Wl,--print-memory-usage)

pico_enable_stdio_uart(kvm_controller 1)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(kvm_controller PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(kvm_controller PRIVATE PIO_USB_USE_TINYUSB)
target_compile_definitions(kvm_controller PRIVATE KVM_CONTROLLER)

# Link the standard library (gpio, timers, sleep_ms, etc.)
target_link_libraries(kvm_controller
        shared
        pico_stdlib
        pico_multicore
        hardware_spi
        pico_pio_usb
        tinyusb_device
        tinyusb_host
        tinyusb_board)

# Produce the .uf2 (plus .elf, .bin, map file)
pico_add_extra_outputs(kvm_controller)