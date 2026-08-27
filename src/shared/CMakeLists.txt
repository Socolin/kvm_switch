add_compile_options(-Wall -Wextra)
add_library(shared
        crc.c
        hid_manager.c
        logger.c
        node_link.c
        pico_utils.c
        report_descriptor.c
        utils.c
)

target_include_directories(shared
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(shared
        pico_stdlib
        hardware_spi
)