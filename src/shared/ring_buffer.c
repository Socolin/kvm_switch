#include "ring_buffer.h"

void ring_buffer_init(
    ring_buffer_t *ring_buffer,
    uint8_t *buffer,
    const size_t buffer_size
) {
    ring_buffer->read_position = 0;
    ring_buffer->write_position = 0;
    ring_buffer->buffer = buffer;
    ring_buffer->buffer_size = buffer_size;
}

size_t ring_buffer_get_length(
    const ring_buffer_t *ring_buffer
) {
    if (ring_buffer->read_position == ring_buffer->write_position)
        return 0;

    if (ring_buffer->read_position < ring_buffer->write_position)
        return ring_buffer->write_position - ring_buffer->read_position;

    return ring_buffer->buffer_size - (ring_buffer->read_position - ring_buffer->write_position);
}

size_t ring_buffer_get_available_space(
    const ring_buffer_t *ring_buffer
) {
    return ring_buffer->buffer_size - 1 - ring_buffer_get_length(ring_buffer);
}

size_t ring_buffer_read(
    ring_buffer_t *ring_buffer,
    uint8_t *bytes,
    const size_t len
) {
    size_t read_count = 0;
    while (read_count < len) {
        if (ring_buffer->read_position == ring_buffer->write_position)
            break;
        if (bytes) {
            bytes[read_count] = ring_buffer->buffer[ring_buffer->read_position];
        }
        read_count++;
        ring_buffer->read_position = (ring_buffer->read_position + 1) % ring_buffer->buffer_size;
    }
    return read_count;
}

size_t ring_buffer_peek(
    const ring_buffer_t *ring_buffer,
    uint8_t *bytes,
    const size_t len
) {
    size_t read_count = 0;
    size_t peek_position = ring_buffer->read_position;
    while (read_count < len) {
        if (peek_position == ring_buffer->write_position)
            break;
        if (bytes) {
            bytes[read_count] = ring_buffer->buffer[peek_position];
        }
        read_count++;
        peek_position = (peek_position + 1) % ring_buffer->buffer_size;
    }
    return read_count;
}

size_t ring_buffer_write(
    ring_buffer_t *ring_buffer,
    const uint8_t *bytes,
    const size_t len
) {
    size_t written = 0;
    while (written < len) {
        const size_t next_write_position = (ring_buffer->write_position + 1) % ring_buffer->buffer_size;
        if (next_write_position == ring_buffer->read_position)
            break;
        ring_buffer->buffer[ring_buffer->write_position] = bytes[written];
        written++;
        ring_buffer->write_position = next_write_position;
    }
    return written;
}

