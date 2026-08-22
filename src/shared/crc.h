#pragma once

#include <stdint.h>

uint16_t crc16_ccitt_false(
    const uint8_t data[],
    uint16_t data_len
);

uint16_t crc16_ccitt_false_continue(
    uint16_t crc,
    const uint8_t data[],
    uint16_t data_len
);
