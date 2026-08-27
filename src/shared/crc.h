#pragma once

#include <stdint.h>

uint16_t crc16_ccitt_false(
    const uint8_t data[],
    uint16_t data_len
);
