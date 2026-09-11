#pragma once
#include <stdint.h>


// 6.2.2.2 Short Items

typedef enum {
    SHORT_ITEM_SIZE_0BYTES,
    SHORT_ITEM_SIZE_1BYTES,
    SHORT_ITEM_SIZE_2BYTES,
    SHORT_ITEM_SIZE_4BYTES,
} hid_report_short_item_size_t;

typedef enum {
    SHORT_ITEM_TYPE_MAIN,
    SHORT_ITEM_TYPE_GLOBAL,
    SHORT_ITEM_TYPE_LOCAL,
} hid_report_short_item_type_t;

// 6.2.2.4 Main Items

typedef enum {
    HID_REPORT_MAIN_ITEM_INPUT = 8,
    HID_REPORT_MAIN_ITEM_OUTPUT = 9,
    HID_REPORT_MAIN_ITEM_COLLECTION = 10,
    HID_REPORT_MAIN_ITEM_FEATURE = 11,
    HID_REPORT_MAIN_ITEM_COLLECTION_END = 12,
} hid_report_main_item_tag_t;

// 6.2.2.5 Main Items

typedef struct __attribute__((packed)) {
    bool data_constant: 1;
    bool array_variable: 1;
    bool absolute_relative: 1;
    bool no_wrap_wrap: 1;
    bool linear_non_linear: 1;
    bool preferred_state_no_preferred: 1;
    bool no_null_position_null_state: 1;
    bool non_volatile_volatile: 1;
    bool bit_field_buffered_bytes: 1;
    uint32_t reserved2: 23;
} hid_report_main_item_flags_t;

#define HID_MAIN_ITEM_IS_DATA(x) (!(x)->data_constant)
#define HID_MAIN_ITEM_IS_CONSTANT(x) ((x)->data_constant)

#define HID_MAIN_ITEM_IS_ARRAY(x) ((x)->array_variable)
#define HID_MAIN_ITEM_IS_VARIABLE(x) ((x)->array_variable)

#define HID_MAIN_ITEM_IS_ABSOLUTE(x) ((x)->absolute_relative)
#define HID_MAIN_ITEM_IS_RELATIVE(x) (!(x)->absolute_relative)

// 6.2.2.6 Collection, End Collection Items

typedef enum {
    HID_REPORT_COLLECTION_ITEM_PHYSICAL = 0,
    HID_REPORT_COLLECTION_ITEM_APPLICATION = 1,
    HID_REPORT_COLLECTION_ITEM_LOGICAL = 2,
    HID_REPORT_COLLECTION_ITEM_REPORT = 3,
    HID_REPORT_COLLECTION_ITEM_NAMED_ARRAY = 4,
    HID_REPORT_COLLECTION_ITEM_USAGE_SWITCH = 5,
    HID_REPORT_COLLECTION_ITEM_USAGE_MODIFIER = 6,
} hid_report_collection_type;

// 6.2.2.7 Global Items

typedef enum {
    HID_REPORT_GLOBAL_ITEM_USAGE_PAGE = 0,
    HID_REPORT_GLOBAL_ITEM_LOGICAL_MIN = 1,
    HID_REPORT_GLOBAL_ITEM_LOGICAL_MAX = 2,
    HID_REPORT_GLOBAL_ITEM_PHYSICAL_MIN = 3,
    HID_REPORT_GLOBAL_ITEM_PHYSICAL_MAX = 4,
    HID_REPORT_GLOBAL_ITEM_UNIT_EXPONENT = 5,
    HID_REPORT_GLOBAL_ITEM_UNIT = 6,
    HID_REPORT_GLOBAL_ITEM_REPORT_SIZE = 7,
    HID_REPORT_GLOBAL_ITEM_REPORT_ID = 8,
    HID_REPORT_GLOBAL_ITEM_REPORT_COUNT = 9,
    HID_REPORT_GLOBAL_ITEM_PUSH = 10,
    HID_REPORT_GLOBAL_ITEM_POP = 11
} hid_report_global_item_tag_t;

// 6.2.2.8 Local Items

typedef enum {
    HID_REPORT_LOCAL_ITEM_USAGE = 0,
    HID_REPORT_LOCAL_ITEM_USAGE_MIN = 1,
    HID_REPORT_LOCAL_ITEM_USAGE_MAX = 2,
    HID_REPORT_LOCAL_ITEM_DESIGNATOR_INDEX = 3,
    HID_REPORT_LOCAL_ITEM_DESIGNATOR_MIN = 4,
    HID_REPORT_LOCAL_ITEM_DESIGNATOR_MAX = 5,
    HID_REPORT_LOCAL_ITEM_STRING_INDEX = 6,
    HID_REPORT_LOCAL_ITEM_STRING_MIN = 7,
    HID_REPORT_LOCAL_ITEM_STRING_MAX = 8,
    HID_REPORT_LOCAL_ITEM_DELIMITER = 9,
} hid_report_local_item_t;