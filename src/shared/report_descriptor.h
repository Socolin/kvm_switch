#pragma once

#include "hid_report.h"
#include <stdint.h>

#define MAX_REPORTS 16
#define MAX_COLLECTIONS 32
#define MAX_FIELDS 64
#define MAX_USAGES 64

typedef enum {
    HID_REPORT_TYPE_INPUT,
    HID_REPORT_TYPE_OUTPUT,
    HID_REPORT_TYPE_FEATURE,
} hid_report_type_t;

typedef struct {
    int32_t min;
    int32_t max;
} hid_range_t;

typedef struct __attribute((packed)) {
    union {
        hid_report_main_item_flags_t parsed_flags;
        uint32_t flags;
    } flags;

    bool use_usage_range;

    union {
        struct {
            uint16_t min;
            uint16_t max;
        } range;

        uint16_t value;
    } usage;

    uint16_t usage_page;
    hid_range_t logical;
    hid_range_t physical;
    int32_t unit_exponent;
    uint32_t unit;
    uint16_t bit_size;

    int8_t collection_idx;
} hid_field_t;

typedef struct __attribute((packed)) {
    uint8_t report_id;
    hid_report_type_t report_type;

    uint16_t field_count;
    uint16_t *field_indices;
} hid_report_t;

typedef enum {
    HID_COLL_PHYSICAL,
    HID_COLL_APPLICATION,
    HID_COLL_LOGICAL,
    HID_COLL_REPORT,
    HID_COLL_NAMED_ARRAY,
    HID_COLL_USAGE_SWITCH,
    HID_COLL_USAGE_MODIFIER,
} hid_collection_type_t;

typedef struct __attribute((packed)) {
    uint16_t usage_page;
    uint16_t usage;
    hid_collection_type_t type;
    int8_t parent_idx;
} hid_collection_t;

typedef struct __attribute((packed)) {
    uint16_t fields_count;
    uint16_t max_fields;
    uint8_t collection_count;
    uint8_t max_collections;
    uint8_t max_reports;
    uint8_t report_count;
    uint16_t max_field_per_report;

    hid_field_t *fields;
    hid_collection_t *collections;
    hid_report_t *reports;
} hid_descriptor_t;


bool is_report_id_present_in_descriptor(
    const uint8_t *report_desc,
    uint16_t desc_len
);

hid_descriptor_t *parse_report_descriptor(
    const uint8_t *report_desc,
    uint16_t desc_len
);

void print_report_descriptor(
    const hid_descriptor_t *report_descriptor,
    int (*print)(void *user_data, const char *format, ...),
    void *user_data
);
