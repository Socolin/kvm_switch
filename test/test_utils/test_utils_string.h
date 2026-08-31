#pragma once

typedef struct {
    char buffer[4096];
    int length;
} test_print_buffer_t;

int print_to_buffer(
    void *user_data,
    const char *format,
    ...
);
