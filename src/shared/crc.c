#include "crc.h"

uint16_t crc16_ccitt_false(
    const uint8_t *data,
    const uint16_t data_len
) {
    uint16_t crc = 0xffff;

    for (uint16_t i = 0; i < data_len; i++) {
        crc = crc ^ (data[i] << 8);

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }

            crc = crc & 0xFFFF;
        }
    }

    return crc;
}
