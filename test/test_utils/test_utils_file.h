#pragma once

#include <stddef.h>

char *load_file_in_memory(
    const char *file_path
);


void string_combine_impl(
    char *buffer,
    size_t buffer_size,
    ...
);

#define string_combine(buffer, ...) string_combine_impl(buffer, sizeof(buffer), __VA_ARGS__, nullptr)
