#include "report_descriptor.h"


#include <stdlib.h>
#include <string.h>
#include <machine/endian.h>

#include "utils.h"

#define MAX_REPORTS 16
#define MAX_USAGES MAX_REPORTS
#define MAX_FIELDS_PER_REPORT 32

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
    bool reserved1: 1;
    bool bit_field_buffered_bytes: 1;
    uint32_t reserved2: 22;
} hid_report_main_item_data_t;


// > If the Input item is an array, only the Data/Constant, Variable/Array and
// > Absolute/Relative attributes apply.
typedef struct __attribute__((packed)) {
    bool data_constant: 1;
    bool array_variable: 1;
    bool absolute_relative: 1;
    bool ignored1: 1;
    bool ignored2: 1;
    bool ignored3: 1;
    bool ignored4: 1;
    bool ignored5: 1;
    bool reserved1: 1;
    bool ignored6: 1;
    uint32_t reserved2: 22;
} hid_report_main_input_item_data_t;

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

typedef struct {
    uint8_t report_id; // 0 if device uses no IDs
    uint16_t usage_page;
    uint16_t usage_min, usage_max;
    uint32_t bit_offset; // within this report ID's layout
    uint8_t bit_size;
    uint8_t report_count;
    int32_t logical_min, logical_max;
    uint32_t flags; // const/data, array/variable, abs/rel
} hid_field_t;

typedef struct {
    uint16_t usage_page;
    int32_t logical_min;
    int32_t logical_max;
    int32_t physical_min;
    int32_t physical_max;
    int32_t unit_exponent;
    uint32_t unit;
    uint32_t report_size; /**< bits per field */
    uint32_t report_count; /**< number of fields */
    uint8_t report_id; /**< 0 = device uses no report IDs */
} hid_global_state_t;

typedef struct {
    uint16_t usages[MAX_USAGES]; // explicit usage list
    uint8_t usage_count;
    uint16_t usage_min, usage_max; // usage range form
    uint8_t has_usage_range;
    // designator/string indices omitted for brevity
} hid_local_state_t;

typedef struct {
    uint8_t report_id; // 0 if none
    uint8_t type; // INPUT / OUTPUT / FEATURE
    uint32_t total_bits; // running bit length of this report
    hid_field_t fields[MAX_FIELDS_PER_REPORT];
    uint8_t field_count;
} hid_report_t;

typedef struct {
    hid_report_t reports[MAX_REPORTS];
    uint8_t report_count;
    uint8_t uses_report_ids; // 0 => reports[].report_id all 0
} HidReportDescriptor;

// See hid1_11.pdf
bool is_report_id_present_in_descriptor(
    const uint8_t *report_desc,
    const uint16_t desc_len
) {
    uint16_t i = 0;

    while (i < desc_len) {
        const uint8_t prefix = report_desc[i++];

        // Skip unused "Long items"
        if (prefix == 0xFE) {
            if (i + 2 > desc_len) {
                return false;
            }

            const uint8_t data_size = report_desc[i++];
            [[maybe_unused]] const uint8_t long_tag = report_desc[i++];

            if (i + data_size > desc_len) {
                return false;
            }

            i += data_size;
            continue;
        }

        const uint8_t size_code = prefix & 0x3;
        const uint8_t type = (prefix >> 2) & 0x3;
        const uint8_t tag = (prefix >> 4) & 0xf;
        const uint8_t data_size = (1 << size_code) >> 1;

        if (i + data_size > desc_len)
            return false;

        if (type == 1 /*RI_TYPE_GLOBAL*/ && tag == 8 /*RI_GLOBAL_REPORT_ID*/) {
            return true;
        }

        i += data_size;
    }

    return false;
}

#define GLOBAL_STATE_STACK_SIZE 5

typedef struct {
    hid_local_state_t local_state;
    uint8_t global_states_depth;
    hid_global_state_t global_states_stack[GLOBAL_STATE_STACK_SIZE];
} hid_parser_state_t;

static hid_parser_state_t parser;

hid_report_descriptor_t *parse_report_descriptor(
    const uint8_t *report_desc,
    const uint16_t desc_len
) {
    hid_report_descriptor_t *report_descriptor = calloc(1, sizeof(hid_report_descriptor_t));
    hid_global_state_t *global_state = &parser.global_states_stack[0];
    uint16_t i = 0;

    while (i < desc_len) {
        const uint8_t prefix = report_desc[i++];

        if (prefix == 0xFE) {
            if (i + 2 > desc_len) {
                goto error;
            }

            const uint8_t data_size = report_desc[i++];
            [[maybe_unused]] const uint8_t long_tag = report_desc[i++];

            if (i + data_size > desc_len) {
                goto error;
            }

            i += data_size;
            continue;
        }

        const hid_report_short_item_size_t size_code = prefix & 0x3;
        const hid_report_short_item_type_t type = (prefix >> 2) & 0x3;
        const uint8_t tag = (prefix >> 4) & 0xf;
        const uint8_t data_size = (1 << size_code) >> 1;

        if (i + data_size > desc_len)
            goto error;

        uint32_t data = 0;
        memcpy(&data, &report_desc[i + 1], data_size);

        switch (type) {
            case SHORT_ITEM_TYPE_MAIN: {
                switch (tag) {
                    case HID_REPORT_MAIN_ITEM_INPUT: {
                        break;
                    }
                    case HID_REPORT_MAIN_ITEM_OUTPUT: {
                        break;
                    }
                    case HID_REPORT_MAIN_ITEM_COLLECTION: {
                        break;
                    }
                    case HID_REPORT_MAIN_ITEM_FEATURE: {
                        break;
                    }
                    case HID_REPORT_MAIN_ITEM_COLLECTION_END: {
                        break;
                    }
                }
                break;
            }
            case SHORT_ITEM_TYPE_GLOBAL: {
                switch (tag) {
                    case HID_REPORT_GLOBAL_ITEM_USAGE_PAGE: {
                        global_state->usage_page = le32toh(data);
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_LOGICAL_MIN: {
                        global_state->logical_min = le32toh(data);
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_LOGICAL_MAX: {
                        global_state->logical_max = le32toh(data);
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_PHYSICAL_MIN: {
                        global_state->physical_min = le32toh(data);
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_PHYSICAL_MAX: {
                        global_state->physical_max = le32toh(data);
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_UNIT_EXPONENT: {
                        global_state->unit_exponent = le32toh(data);
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_UNIT: {
                        global_state->unit = le32toh(data);
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_REPORT_SIZE: {
                        global_state->report_size = le32toh(data);
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_REPORT_ID: {
                        global_state->report_id = le32toh(data);
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_REPORT_COUNT: {
                        global_state->report_count = le32toh(data);
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_PUSH: {
                        if (parser.global_states_depth >= GLOBAL_STATE_STACK_SIZE) {
                            goto error;
                        }
                        global_state = &parser.global_states_stack[parser.global_states_depth++];
                        memset(global_state, 0, sizeof(hid_global_state_t));
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_POP: {
                        if (parser.global_states_depth <= 0) {
                            goto error;
                        }
                        global_state = &parser.global_states_stack[parser.global_states_depth--];
                        break;
                    }
                    default:
                        goto error;
                }
                break;
            }
            case SHORT_ITEM_TYPE_LOCAL: {
                switch (tag) {
                    case HID_REPORT_LOCAL_ITEM_USAGE: {
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_USAGE_MIN: {
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_USAGE_MAX: {
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DESIGNATOR_INDEX: {
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DESIGNATOR_MIN: {
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DESIGNATOR_MAX: {
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_STRING_INDEX: {
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_STRING_MIN: {
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_STRING_MAX: {
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DELIMITER: {
                        break;
                    }
                    default: {
                        break;
                    }
                }
                break;
            }
            default: {
                goto error;
            }
        }

        i += data_size;
    }
    return report_descriptor;
error:
    free(report_descriptor);
    return nullptr;
}
