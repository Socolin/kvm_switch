add_library(shared
        logger.c
        crc.c
        hid_manager.c
        utils.h
)

target_include_directories(shared
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(shared
        pico_stdlib
)