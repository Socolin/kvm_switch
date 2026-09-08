#pragma once
#include <stddef.h>

void config_persistence_save(
    const void *config_data,
    size_t config_data_len
);

void config_persistence_read(
    const void *config_data,
    size_t config_data_len
);
