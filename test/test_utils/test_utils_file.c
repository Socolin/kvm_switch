#include "test_utils_file.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unity.h"

char *load_file_in_memory(
    const char *file_path
) {
    FILE *file = fopen(file_path, "r");
    TEST_ASSERT(file);
    if (fseek(file, 0L, SEEK_END) != 0) {
        fclose(file);
        TEST_FAIL_MESSAGE("load_file_in_memory:fseek failed");
        return nullptr;
    }
    const size_t file_size = ftell(file);
    if (fseek(file, 0L, SEEK_SET) != 0) {
        TEST_FAIL_MESSAGE("load_file_in_memory:fseek failed");
        fclose(file);
        return nullptr;
    }

    char *buffer = calloc(file_size + 1, sizeof(char));
    if (!buffer) {
        TEST_FAIL_MESSAGE("load_file_in_memory:calloc failed");
        fclose(file);
        return nullptr;
    }

    const size_t r = fread(buffer, sizeof(char), file_size, file);
    if (r != file_size) {
        TEST_FAIL_MESSAGE("load_file_in_memory:fread failed");
        fclose(file);
        free(buffer);
        return nullptr;
    }

    fclose(file);

    return buffer;
}

void string_combine_impl(
    char *buffer,
    const size_t buffer_size,
    ...
) {
    va_list args;
    va_start(args, buffer_size);
    while (true) {
        const char *part = va_arg(args, const char*);
        if (!part)
            break;
        strncat(buffer, part, buffer_size - strlen(buffer) - 1);
    }

    va_end(args);
}
