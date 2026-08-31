#include "report_descriptor.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "hid_report.h"
#include "utils.h"



typedef struct {
    uint16_t usage_page;
    hid_range_t logical;
    hid_range_t physical;
    int32_t unit_exponent;
    uint32_t unit;
    uint32_t report_size; /**< bits per field */
    uint8_t report_count; /**< number of fields */
    uint8_t report_id; /**< 0 = device uses no report IDs */
} hid_global_state_t;

typedef struct {
    uint16_t usage;
    uint16_t usage_page;
    bool is_extended_usage;
    /**< See 6.2.2.8 if a usage is defined as 32 bit in the report descriptor it also include the usage page */
} hid_parsing_usage_t;

typedef struct {
    bool use_range;
    uint16_t usage_count;
    hid_parsing_usage_t usages[MAX_USAGES];
    hid_parsing_usage_t usage_min;
    hid_parsing_usage_t usage_max;
    uint32_t designator_index;
    uint32_t designator_min;
    uint32_t designator_max;
    uint32_t string_index;
    uint32_t string_min;
    uint32_t string_max;
    uint32_t delimiter;
} hid_local_state_t;

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

#define GLOBAL_STATE_STACK_SIZE 4
#define COLLECTION_STACK_SIZE 4

typedef struct {
    hid_local_state_t local_state;
    uint8_t global_states_depth;
    hid_global_state_t global_states_stack[GLOBAL_STATE_STACK_SIZE];
    uint8_t collection_depth;
    int8_t collection_stack[COLLECTION_STACK_SIZE];
} hid_parser_state_t;


static hid_report_t *reserve_report(
    hid_descriptor_t *report_descriptor
) {
    if (report_descriptor->report_count >= MAX_REPORTS) {
        return nullptr;
    }
    hid_report_t *field = &report_descriptor->reports[report_descriptor->report_count];
    report_descriptor->report_count++;
    return field;
}

static hid_report_t *reserve_or_get_report(
    hid_descriptor_t *report_descriptor,
    const uint8_t report_id,
    const hid_report_type_t report_type
) {
    for (int i = 0; i < report_descriptor->report_count; i++) {
        hid_report_t *report = &report_descriptor->reports[i];
        if (report->report_id == report_id && report->report_type == report_type) {
            return report;
        }
    }
    hid_report_t *report = reserve_report(report_descriptor);
    if (report == nullptr) {
        return nullptr;
    }
    report->report_id = report_id;
    report->report_type = report_type;
    return report;
}

static bool add_field_to_report(
    hid_report_t *report,
    const uint8_t field_idx
) {
    if (report->field_count > MAX_FIELDS)
        return false;

    report->field_indices[report->field_count++] = field_idx;
    return true;
}

static hid_collection_t *reserve_collection(
    hid_descriptor_t *report_descriptor,
    int8_t *out_collection_idx
) {
    if (report_descriptor->collection_count >= MAX_COLLECTIONS) {
        return nullptr;
    }
    hid_collection_t *collection = &report_descriptor->collections[report_descriptor->collection_count];
    *out_collection_idx = (int8_t) report_descriptor->collection_count;
    report_descriptor->collection_count++;
    return collection;
}

static hid_field_t *reserve_field(
    hid_descriptor_t *report_descriptor,
    const uint8_t report_id,
    const hid_report_type_t report_type
) {
    if (report_descriptor->field_count >= MAX_FIELDS) {
        return nullptr;
    }
    hid_field_t *field = &report_descriptor->fields[report_descriptor->field_count];
    hid_report_t *report = reserve_or_get_report(report_descriptor, report_id, report_type);
    if (!report) {
        return nullptr;
    }
    if (!add_field_to_report(report, report_descriptor->field_count)) {
        return nullptr;
    }
    report_descriptor->field_count++;
    return field;
}


static hid_report_type_t get_report_type_from_tag(uint8_t tag) {
    switch (tag) {
        case HID_REPORT_MAIN_ITEM_OUTPUT:
            return HID_REPORT_TYPE_OUTPUT;
        case HID_REPORT_MAIN_ITEM_FEATURE:
            return HID_REPORT_TYPE_FEATURE;
        case HID_REPORT_MAIN_ITEM_INPUT:
            return HID_REPORT_TYPE_INPUT;
        default:
            assert(false);
    }
}

static int32_t parse_signed_item_value(
    const uint8_t *data,
    const uint8_t data_size
) {
    if (data_size == 1) return le32toh((int8_t) data[0]);
    if (data_size == 2) return le32toh((int16_t) (data[0] | (data[1] << 8)));
    if (data_size == 4) return le32toh(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
    return 0;
}

static uint32_t parse_unsigned_item_value(
    const uint8_t *data,
    const uint8_t data_size
) {
    uint32_t udata = 0;
    memcpy(&udata, data, data_size);
    return le32toh(udata);
}

static void parse_usage(
    hid_parsing_usage_t *usage,
    const uint32_t udata,
    const uint8_t data_size
) {
    usage->is_extended_usage = data_size == 4;
    if (usage->is_extended_usage) {
        usage->usage_page = udata >> 16;
        usage->usage = udata & 0xFFFF;
    } else {
        usage->usage = udata;
    }
}

hid_descriptor_t *parse_report_descriptor(
    const uint8_t *report_desc,
    const uint16_t desc_len
) {
    hid_parser_state_t parser = {0};

    hid_descriptor_t *report_descriptor = calloc(1, sizeof(hid_descriptor_t));
    hid_global_state_t *global_state = &parser.global_states_stack[0];
    hid_local_state_t *local_state = &parser.local_state;
    uint16_t i = 0;
    int8_t active_collection_idx = -1;

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

        const uint8_t *data_start = &report_desc[i];
        const uint32_t udata = parse_unsigned_item_value(data_start, data_size);
        const int32_t sdata = parse_signed_item_value(data_start, data_size);

        switch (type) {
            case SHORT_ITEM_TYPE_MAIN: {
                switch (tag) {
                    case HID_REPORT_MAIN_ITEM_OUTPUT:
                    case HID_REPORT_MAIN_ITEM_FEATURE:
                    case HID_REPORT_MAIN_ITEM_INPUT: {
                        const hid_report_main_item_flags_t *flags = (hid_report_main_item_flags_t *) &udata;
                        const uint8_t report_id = global_state->report_id;
                        const hid_report_type_t report_type = get_report_type_from_tag(tag);

                        if (HID_MAIN_ITEM_IS_CONSTANT(flags)) {
                            hid_field_t *field = reserve_field(report_descriptor, report_id, report_type);
                            if (field == nullptr) {
                                goto error;
                            }
                            field->collection_idx = active_collection_idx;
                            field->bit_size = global_state->report_size * global_state->report_count;
                            field->flags.flags = udata;
                        } else {
                            if (HID_MAIN_ITEM_IS_VARIABLE(flags)) {
                                for (int32_t f = 0; f < global_state->report_count; f++) {
                                    uint16_t usage = 0;
                                    uint16_t usage_page = global_state->usage_page;
                                    if (local_state->use_range) {
                                        usage = min(local_state->usage_min.usage + f, local_state->usage_max.usage);
                                        if (local_state->usage_min.is_extended_usage) {
                                            usage_page = local_state->usage_min.usage_page;
                                        }
                                    } else {
                                        hid_parsing_usage_t parsing_usage = local_state->usages[min(
                                            f, local_state->usage_count - 1)];
                                        usage = parsing_usage.usage;
                                        if (parsing_usage.is_extended_usage) {
                                            usage_page = parsing_usage.usage_page;
                                        }
                                    }
                                    hid_field_t *field = reserve_field(report_descriptor, report_id, report_type);
                                    if (field == nullptr) {
                                        goto error;
                                    }
                                    field->collection_idx = active_collection_idx;
                                    field->bit_size = global_state->report_size;
                                    field->use_usage_range = false;
                                    field->usage.value = usage;
                                    field->usage_page = usage_page;
                                    field->logical = global_state->logical;
                                    field->physical = global_state->physical;
                                    field->unit_exponent = global_state->unit_exponent;
                                    field->unit = global_state->unit;
                                    field->flags.flags = udata;
                                }
                            } else {
                                for (uint32_t f = 0; f < global_state->report_count; f++) {
                                    hid_field_t *field = reserve_field(report_descriptor, report_id, report_type);
                                    if (field == nullptr) {
                                        goto error;
                                    }
                                    field->collection_idx = active_collection_idx;
                                    field->bit_size = global_state->report_size;
                                    field->use_usage_range = true;
                                    field->usage.range.min = local_state->usage_min.usage;
                                    field->usage.range.max = local_state->usage_max.usage;
                                    if (local_state->usage_min.is_extended_usage) {
                                        field->usage_page = local_state->usage_min.usage_page;
                                    } else {
                                        field->usage_page = global_state->usage_page;
                                    }
                                    field->logical = global_state->logical;
                                    field->physical = global_state->physical;
                                    field->unit_exponent = global_state->unit_exponent;
                                    field->unit = global_state->unit;
                                    field->flags.flags = udata;
                                }
                            }
                        }
                        break;
                    }
                    case HID_REPORT_MAIN_ITEM_COLLECTION: {
                        if (parser.collection_depth >= COLLECTION_STACK_SIZE) {
                            goto error;
                        }

                        int8_t collection_id;
                        hid_collection_t *collection = reserve_collection(report_descriptor, &collection_id);
                        if (collection == nullptr) {
                            goto error;
                        }

                        collection->parent_idx = active_collection_idx;
                        collection->usage_page = global_state->usage_page;
                        collection->type = udata;
                        if (local_state->usage_count > 0) {
                            collection->usage = local_state->usages[0].usage;
                            if (local_state->usages[0].is_extended_usage) {
                                collection->usage_page = local_state->usages[0].usage_page;
                            }
                        }

                        parser.collection_stack[parser.collection_depth++] = active_collection_idx;
                        active_collection_idx = collection_id;

                        break;
                    }
                    case HID_REPORT_MAIN_ITEM_COLLECTION_END: {
                        if (parser.collection_depth <= 0) {
                            goto error;
                        }
                        active_collection_idx = parser.collection_stack[--parser.collection_depth];
                        break;
                    }
                    default: {
                        goto error;
                        break;
                    }
                }
                memset(local_state, 0, sizeof(*local_state));
                break;
            }
            case SHORT_ITEM_TYPE_GLOBAL: {
                switch (tag) {
                    case HID_REPORT_GLOBAL_ITEM_USAGE_PAGE: {
                        global_state->usage_page = udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_LOGICAL_MIN: {
                        global_state->logical.min = sdata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_LOGICAL_MAX: {
                        global_state->logical.max = sdata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_PHYSICAL_MIN: {
                        global_state->physical.min = sdata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_PHYSICAL_MAX: {
                        global_state->physical.max = sdata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_UNIT_EXPONENT: {
                        const int32_t code = sdata & 0xf;
                        if (code >= 0x8) {
                            global_state->unit_exponent = -16 + code;
                        } else {
                            global_state->unit_exponent = code;
                        }
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_UNIT: {
                        global_state->unit = udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_REPORT_SIZE: {
                        global_state->report_size = udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_REPORT_ID: {
                        global_state->report_id = udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_REPORT_COUNT: {
                        global_state->report_count = udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_PUSH: {
                        if (parser.global_states_depth >= GLOBAL_STATE_STACK_SIZE - 1) {
                            goto error;
                        }
                        const hid_global_state_t *actual_global_state = global_state;
                        global_state = &parser.global_states_stack[++parser.global_states_depth];
                        memcpy(global_state, actual_global_state, sizeof(hid_global_state_t));
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_POP: {
                        if (parser.global_states_depth <= 0) {
                            goto error;
                        }
                        global_state = &parser.global_states_stack[--parser.global_states_depth];
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
                        if (local_state->usage_count > MAX_USAGES)
                            goto error;
                        local_state->use_range = false;
                        parse_usage(&local_state->usages[local_state->usage_count], udata, data_size);
                        local_state->usage_count++;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_USAGE_MIN: {
                        local_state->use_range = true;
                        parse_usage(&local_state->usage_min, udata, data_size);
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_USAGE_MAX: {
                        local_state->use_range = true;
                        parse_usage(&local_state->usage_max, udata, data_size);
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DESIGNATOR_INDEX: {
                        local_state->designator_index = (int32_t) udata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DESIGNATOR_MIN: {
                        local_state->designator_min = (int32_t) udata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DESIGNATOR_MAX: {
                        local_state->designator_max = (int32_t) udata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_STRING_INDEX: {
                        local_state->string_index = (int32_t) udata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_STRING_MIN: {
                        local_state->string_min = (int32_t) udata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_STRING_MAX: {
                        local_state->string_max = (int32_t) udata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DELIMITER: {
                        local_state->delimiter = (int32_t) udata;
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

static const char *report_type_to_string(
    const uint8_t report_type
) {
    switch (report_type) {
        case HID_REPORT_TYPE_INPUT:
            return "Input";
        case HID_REPORT_TYPE_OUTPUT:
            return "Output";
        case HID_REPORT_TYPE_FEATURE:
            return "Feature";
        default:
            return "Unknown";
    }
}

static const char *collection_type_to_string(
    const uint8_t collection_type
) {
    switch (collection_type) {
        case HID_COLL_PHYSICAL:
            return "Physical";
        case HID_COLL_APPLICATION:
            return "Application";
        case HID_COLL_LOGICAL:
            return "Logical";
        case HID_COLL_REPORT:
            return "Report";
        case HID_COLL_NAMED_ARRAY:
            return "Named Array";
        case HID_COLL_USAGE_SWITCH:
            return "Usage Switch";
        case HID_COLL_USAGE_MODIFIER:
            return "Usage Modifier";
        default:
            return "Unknown";
    }
}

void print_report_descriptor(
    const hid_descriptor_t *report_descriptor,
    int (*print)(void *user_data, const char *format, ...) __attribute__((format(printf, 2, 3))),
    void *user_data
) {
    print(user_data, "Collections:\n");
    for (uint32_t c = 0; c < report_descriptor->collection_count; c++) {
        const hid_collection_t *collection = &report_descriptor->collections[c];
        print(user_data, "  [%" PRIu32 "] %s", c, collection_type_to_string(collection->type));
        print(user_data, ", UsagePage: %" PRIu32, collection->usage_page);
        print(user_data, ", Usage: %" PRId32, collection->usage);
        print(user_data, ", Parent: %" PRId8 "\n", collection->parent_idx);
    }
    for (uint32_t r = 0; r < report_descriptor->report_count; r++) {
        const hid_report_t *report = &report_descriptor->reports[r];
        print(user_data, "Report %" PRIu32 ":\n", r);
        print(user_data, "  Report ID: %" PRIu8 "\n", report->report_id);
        print(user_data, "  Report Type: %s\n", report_type_to_string(report->report_type));
        print(user_data, "  Fields:\n");
        for (uint32_t f = 0; f < report->field_count; f++) {
            const hid_field_t *field = &report_descriptor->fields[report->field_indices[f]];
            print(user_data, "    Bits: %u", field->bit_size);
            print(user_data, ", Collection: %" PRId8, field->collection_idx);
            print(user_data, ", UsagePage: %" PRIu16, field->usage_page);
            if (field->use_usage_range) {
                print(user_data, ", Usage: %" PRId32 "-%" PRId32, field->usage.range.min, field->usage.range.max);
            } else {
                print(user_data, ", Usage: %" PRId32, field->usage.value);
            }
            print(user_data, ", Logical Minimum: %" PRId32, field->logical.min);
            print(user_data, ", Logical Maximum: %" PRId32, field->logical.max);
            if (field->physical.min != 0 && field->physical.max != 0) {
                print(user_data, ", Physical Minimum: %" PRId32, field->physical.min);
                print(user_data, ", Physical Maximum: %" PRId32, field->physical.max);
                print(user_data, ", Unit Exponent: %" PRId32, field->unit_exponent);
                print(user_data, ", Unit: 0x%" PRIx32, field->unit);
            }
            print(user_data, ", Flags: 0x%02" PRIx32, field->flags.flags);
            print(user_data, " (");
            if (field->flags.parsed_flags.data_constant)
                print(user_data, "Constant");
            else
                print(user_data, "Data");
            print(user_data, ",");
            if (field->flags.parsed_flags.array_variable)
                print(user_data, "Variable");
            else
                print(user_data, "Array");
            print(user_data, ",");
            if (field->flags.parsed_flags.absolute_relative)
                print(user_data, "Relative");
            else
                print(user_data, "Absolute");
            if (report->report_type != HID_REPORT_TYPE_INPUT && HID_MAIN_ITEM_IS_ARRAY(&field->flags.parsed_flags)) {
                print(user_data, ",");
                if (field->flags.parsed_flags.no_wrap_wrap)
                    print(user_data, "Wrap");
                else
                    print(user_data, "No Wrap");
                print(user_data, ",");
                if (field->flags.parsed_flags.linear_non_linear)
                    print(user_data, "Non Linear");
                else
                    print(user_data, "Linear");
                print(user_data, ",");
                if (field->flags.parsed_flags.preferred_state_no_preferred)
                    print(user_data, "No Preferred State");
                else
                    print(user_data, "Preferred State");
                print(user_data, ",");
                if (field->flags.parsed_flags.non_volatile_volatile)
                    print(user_data, "Volatile");
                else
                    print(user_data, "Non Volatile");
                print(user_data, ",");
                if (field->flags.parsed_flags.bit_field_buffered_bytes)
                    print(user_data, "No Buffered Bytes");
                else
                    print(user_data, "Buffered Bytes");
            }
            print(user_data, ")");
            print(user_data, "\n");
        }
    }
}
