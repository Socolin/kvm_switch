add_compile_options(-Wall -Wextra)
add_library(shared
        crc.c
        gpio_utils.c
        hid_keyboard_report_util.c
        hid_manager.c
        hid_report_descriptor.c
        logger.c
        node_link.c
        pico_utils.c
        ring_buffer.c
        quick_reset_button.c
        utils.c
        version.h
)

target_include_directories(shared
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(shared
        pico_stdlib
        hardware_spi
        hardware_gpio
)