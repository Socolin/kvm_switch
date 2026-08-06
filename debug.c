#include "debug.h"
#include <stdio.h>

void debug_print_buffer(
    const uint8_t *data,
    const size_t data_len
) {
    for (size_t i = 0; i < data_len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}
