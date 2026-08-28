#pragma once

#include "pico/types.h"

typedef void (*gpio_handler_t)(uint gpio, uint32_t event_mask, const void *user_data);

void gpio_util_set_handler(
    uint gpio,
    uint32_t event_mask,
    gpio_handler_t handler,
    const void *user_data
);

void gpio_util_clear_handler(
    uint gpio
);
