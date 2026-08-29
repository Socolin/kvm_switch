#pragma once

#include <stdint.h>

typedef struct {
    uint8_t report_id;
    uint16_t usage_page;
    uint16_t usage_min;
    uint16_t usage_max;
    uint32_t bit_offset;
    uint8_t bit_size;
    uint8_t report_count;
    int32_t logical_min, logical_max;
    uint32_t flags;
} hid_report_field_t;

typedef struct {
    uint8_t fields_count;
    hid_report_field_t *fields;
} hid_report_descriptor_t;

bool is_report_id_present_in_descriptor(
    const uint8_t *report_desc,
    uint16_t desc_len
);

hid_report_descriptor_t *parse_report_descriptor(
    const uint8_t *report_desc,
    uint16_t desc_len
);
