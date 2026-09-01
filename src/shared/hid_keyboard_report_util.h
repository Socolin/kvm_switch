#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hid_report_descriptor.h"

bool hid_report_descriptor_contains_keycodes(
    const hid_report_descriptor_t *report_descriptor
);

size_t hid_report_get_pressed_keys(
    const hid_report_descriptor_t *report_descriptor,
    const uint8_t *report,
    uint16_t report_len,
    uint8_t hid_report_id,
    uint8_t *pressed_keys,
    size_t pressed_keys_len
);

size_t hid_report_keyboard_boot_get_pressed_keys(
    const uint8_t *report,
    uint16_t report_len,
    uint8_t *pressed_keys,
    size_t pressed_keys_len
);
