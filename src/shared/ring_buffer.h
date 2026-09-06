#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t read_position;
    size_t write_position;
    size_t buffer_size;
    uint8_t *buffer;
} ring_buffer_t;

void ring_buffer_init(
    ring_buffer_t *ring_buffer,
    uint8_t *buffer,
    size_t buffer_size
);

size_t ring_buffer_get_length(
    const ring_buffer_t *ring_buffer
);

size_t ring_buffer_get_available_space(
    const ring_buffer_t *ring_buffer
);

/**
 * Read data from the ring buffer and remove it
 * @param ring_buffer the ring buffer
 * @param bytes buffer where to write the read data. Can be NULL if the data is not needed
 * @param len number of bytes to read and removed from the buffer
 * @return number of bytes read
 */
size_t ring_buffer_read(
    ring_buffer_t *ring_buffer,
    uint8_t *bytes,
    size_t len
);

/**
 * Copy data from the ring buffer without removing it
 * @param ring_buffer the ring buffer
 * @param bytes buffer where to write the read data. Can be NULL if the data is not needed
 * @param len number of bytes to read.
 * @return number of bytes read
 */
size_t ring_buffer_peek(
    const ring_buffer_t *ring_buffer,
    uint8_t *bytes,
    size_t len
);

size_t ring_buffer_write(
    ring_buffer_t *ring_buffer,
    const uint8_t *bytes,
    size_t len
);
