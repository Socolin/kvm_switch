#include <stdio.h>
#include <stdarg.h>

#include "test_utils_string.h"

int print_to_buffer(
    void *user_data,
    const char *format,
    ...
) {
    test_print_buffer_t *buffer = user_data;
    va_list args;
    va_start(args, format);
    const int message_len = vsnprintf(
        buffer->buffer + buffer->length,
        sizeof(buffer->buffer) - buffer->length,
        format,
        args
    );
    if (message_len + buffer->length >= sizeof(buffer->buffer)) {
        return 0;
    }
    buffer->length += message_len;
    va_end(args);
    return message_len;
}
