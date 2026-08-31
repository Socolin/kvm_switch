#include "hid_report_descriptor.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
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

#define GLOBAL_STATE_STACK_SIZE 4
#define COLLECTION_STACK_SIZE 4

typedef struct {
    hid_local_state_t local_state;
    uint8_t global_states_depth;
    hid_global_state_t global_states_stack[GLOBAL_STATE_STACK_SIZE];
    uint8_t collection_depth;
    int8_t collection_stack[COLLECTION_STACK_SIZE];
} hid_parser_state_t;

typedef struct {
    hid_report_short_item_type_t item_type;
    uint8_t tag;
    uint8_t data_size;
    int32_t sdata;
    uint32_t udata;
} hid_raw_short_item_t;

typedef enum {
    HID_ITEM_PARSE_EOF,
    HID_ITEM_PARSE_SUCCESS,
    HID_ITEM_PARSE_ERROR
} item_parse_result_t;

typedef struct {
    uint16_t field_count;
    uint16_t max_field_per_report;
    uint8_t collection_count;
    uint8_t report_count;
} hid_descriptor_summary_t;

typedef enum {
    HID_REPORT_TYPE_INPUT,
    HID_REPORT_TYPE_OUTPUT,
    HID_REPORT_TYPE_FEATURE,
} hid_report_type_t;

typedef struct {
    uint8_t report_id;
    hid_report_type_t report_type;
    size_t field_count;
} hid_report_definition_t;


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

/**
 * This parse the next short item and advance position. Long items are ignored (they seems to be useless).
 */
static item_parse_result_t try_parse_next_item(
    const uint8_t *report_desc,
    const uint16_t report_desc_len,
    size_t *position,
    hid_raw_short_item_t *out_item
) {
    while (*position < report_desc_len) {
        const uint8_t prefix = report_desc[(*position)++];

        if (prefix == 0xFE) {
            if (*position + 2 > report_desc_len) {
                return HID_ITEM_PARSE_ERROR;
            }

            const uint8_t data_size = report_desc[(*position)++];
            [[maybe_unused]] const uint8_t long_tag = report_desc[(*position)++];

            if (*position + data_size > report_desc_len) {
                return HID_ITEM_PARSE_ERROR;
            }

            *position = *position + data_size;
            continue;
        }

        const hid_report_short_item_size_t size_code = prefix & 0x3;
        const hid_report_short_item_type_t item_type = (prefix >> 2) & 0x3;
        const uint8_t tag = (prefix >> 4) & 0xf;
        const uint8_t data_size = (1 << size_code) >> 1;

        if (*position + data_size > report_desc_len)
            return HID_ITEM_PARSE_ERROR;

        const uint8_t *data_start = &report_desc[*position];

        out_item->item_type = item_type;
        out_item->tag = tag;
        out_item->data_size = data_size;
        out_item->sdata = parse_signed_item_value(data_start, data_size);
        out_item->udata = parse_unsigned_item_value(data_start, data_size);

        *position = *position + data_size;
        return HID_ITEM_PARSE_SUCCESS;
    }
    return HID_ITEM_PARSE_EOF;
}

bool hid_report_descriptor_is_report_id_present(
    const uint8_t *report_desc,
    const uint16_t desc_len
) {
    size_t position = 0;
    hid_raw_short_item_t item;
    while (true) {
        const item_parse_result_t result = try_parse_next_item(report_desc, desc_len, &position, &item);
        if (result == HID_ITEM_PARSE_EOF) {
            break;
        }
        if (result == HID_ITEM_PARSE_ERROR) {
            return false;
        }
        if (item.item_type == SHORT_ITEM_TYPE_GLOBAL && item.tag == HID_REPORT_GLOBAL_ITEM_REPORT_ID) {
            return true;
        }
    }

    return false;
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

/**
 * This partially parses the report descriptor to count the number of required elements to be able to allocate a
 * report with the minimum size.
 */
static bool hid_report_descriptor_count_required_elements(
    const uint8_t *report_desc,
    const uint16_t desc_len,
    hid_descriptor_summary_t *summary
) {
    hid_parser_state_t parser = {0};
    hid_global_state_t *global_state = &parser.global_states_stack[0];
    hid_local_state_t *local_state = &parser.local_state;

    hid_report_definition_t reports_defs[256] = {0};

    size_t position = 0;
    hid_raw_short_item_t item;
    item_parse_result_t item_parse_result;
    while ((item_parse_result = try_parse_next_item(report_desc, desc_len, &position, &item))) {
        if (item_parse_result == HID_ITEM_PARSE_ERROR) {
            return false;
        }
        switch (item.item_type) {
            case SHORT_ITEM_TYPE_MAIN: {
                switch (item.tag) {
                    case HID_REPORT_MAIN_ITEM_OUTPUT:
                    case HID_REPORT_MAIN_ITEM_FEATURE:
                    case HID_REPORT_MAIN_ITEM_INPUT: {
                        const hid_report_main_item_flags_t *flags = (hid_report_main_item_flags_t *) &item.udata;
                        const uint8_t report_id = global_state->report_id;
                        const hid_report_type_t report_type = get_report_type_from_tag(item.tag);

                        hid_report_definition_t *report = nullptr;
                        for (uint16_t i = 0; i < summary->report_count; i++) {
                            if (reports_defs[i].report_id == report_id && reports_defs[i].report_type == report_type) {
                                report = &reports_defs[i];
                                break;
                            }
                        }
                        if (!report) {
                            report = &reports_defs[summary->report_count++];
                            report->report_id = report_id;
                            report->report_type = report_type;
                        }

                        if (HID_MAIN_ITEM_IS_CONSTANT(flags)) {
                            summary->field_count++;
                            report->field_count++;
                        } else {
                            summary->field_count += global_state->report_count;
                            report->field_count += global_state->report_count;
                        }
                        break;
                    }
                    case HID_REPORT_MAIN_ITEM_COLLECTION: {
                        summary->collection_count++;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                memset(local_state, 0, sizeof(*local_state));
                break;
            }
            case SHORT_ITEM_TYPE_GLOBAL: {
                switch (item.tag) {
                    case HID_REPORT_GLOBAL_ITEM_REPORT_SIZE: {
                        global_state->report_size = item.udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_REPORT_ID: {
                        global_state->report_id = item.udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_REPORT_COUNT: {
                        global_state->report_count = item.udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_PUSH: {
                        if (parser.global_states_depth >= GLOBAL_STATE_STACK_SIZE - 1) {
                            return false;
                        }
                        const hid_global_state_t *actual_global_state = global_state;
                        global_state = &parser.global_states_stack[++parser.global_states_depth];
                        memcpy(global_state, actual_global_state, sizeof(hid_global_state_t));
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_POP: {
                        if (parser.global_states_depth <= 0) {
                            return false;
                        }
                        global_state = &parser.global_states_stack[--parser.global_states_depth];
                        break;
                    }
                    default: {
                        break;
                    }
                }
                break;
            }
            default: {
                break;
            }
        }
    }

    summary->max_field_per_report = 0;
    for (uint16_t i = 0; i < summary->report_count; i++) {
        summary->max_field_per_report = max32(summary->max_field_per_report, reports_defs[i].field_count);
    }
    return true;
}

static hid_report_t *reserve_report(
    hid_report_descriptor_t *report_descriptor
) {
    if (report_descriptor->report_count >= report_descriptor->max_reports) {
        return nullptr;
    }
    hid_report_t *field = &report_descriptor->reports[report_descriptor->report_count];
    report_descriptor->report_count++;
    return field;
}

static hid_report_t *reserve_or_get_report(
    hid_report_descriptor_t *report_descriptor,
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
    const hid_report_descriptor_t *report_descriptor,
    hid_report_t *report,
    const uint16_t field_idx
) {
    if (report->field_count >= report_descriptor->max_field_per_report)
        return false;

    report->field_indices[report->field_count++] = field_idx;
    return true;
}

static hid_collection_t *reserve_collection(
    hid_report_descriptor_t *report_descriptor,
    int8_t *out_collection_idx
) {
    if (report_descriptor->collection_count >= report_descriptor->max_collections) {
        return nullptr;
    }
    hid_collection_t *collection = &report_descriptor->collections[report_descriptor->collection_count];
    *out_collection_idx = (int8_t) report_descriptor->collection_count;
    report_descriptor->collection_count++;
    return collection;
}

static hid_field_t *reserve_field(
    hid_report_descriptor_t *report_descriptor,
    const uint8_t report_id,
    const hid_report_type_t report_type
) {
    if (report_descriptor->fields_count >= report_descriptor->max_fields) {
        return nullptr;
    }
    hid_field_t *field = &report_descriptor->fields[report_descriptor->fields_count];
    hid_report_t *report = reserve_or_get_report(report_descriptor, report_id, report_type);
    if (!report) {
        return nullptr;
    }
    if (!add_field_to_report(report_descriptor, report, report_descriptor->fields_count)) {
        return nullptr;
    }
    report_descriptor->fields_count++;
    return field;
}

static void parse_usage(
    hid_parsing_usage_t *usage,
    const hid_raw_short_item_t *item
) {
    usage->is_extended_usage = item->data_size == 4;
    if (usage->is_extended_usage) {
        usage->usage_page = item->udata >> 16;
        usage->usage = item->udata & 0xFFFF;
    } else {
        usage->usage = item->udata;
    }
}

/**
 * Based on the number of fields, collections and reports, detected during pre-parse step, allocate a HID descriptor
 * with the appropriate size.
 */
static hid_report_descriptor_t *hid_descriptor_new(
    const hid_descriptor_summary_t *summary
) {
    hid_report_descriptor_t *report_descriptor = calloc(1, sizeof(hid_report_descriptor_t));
    if (report_descriptor == nullptr)
        return nullptr;

    report_descriptor->max_fields = summary->field_count;
    report_descriptor->fields = calloc(summary->field_count, sizeof(hid_field_t));
    if (report_descriptor->fields == nullptr)
        return nullptr;

    report_descriptor->max_collections = summary->collection_count;
    report_descriptor->collections = calloc(summary->collection_count, sizeof(hid_collection_t));
    if (report_descriptor->collections == nullptr)
        return nullptr;

    const size_t reports_size = summary->report_count * sizeof(hid_report_t);
    const size_t reports_fields_size = summary->report_count * summary->max_field_per_report * sizeof(uint16_t);

    void *reports_block = calloc(1, reports_size + reports_fields_size);
    if (reports_block == nullptr)
        return nullptr;

    const size_t report_fields_size = summary->max_field_per_report * sizeof(uint16_t);
    report_descriptor->max_reports = summary->report_count;
    report_descriptor->max_field_per_report = summary->max_field_per_report;
    report_descriptor->reports = reports_block;

    for (uint16_t i = 0; i < summary->report_count; i++) {
        report_descriptor->reports[i].field_indices = reports_block
                                                      + sizeof(hid_report_t) * summary->report_count
                                                      + i * report_fields_size;
    }

    return report_descriptor;
}

void hid_report_descriptor_free(
    hid_report_descriptor_t *descriptor
) {
    if (descriptor == nullptr) {
        return;
    }

    free(descriptor->fields);
    free(descriptor->collections);
    free(descriptor->reports);
    free(descriptor);
}

hid_report_descriptor_t *hid_report_descriptor_parse(
    const uint8_t *report_desc,
    const uint16_t report_desc_len
) {
    hid_descriptor_summary_t summary = {0};
    if (!hid_report_descriptor_count_required_elements(report_desc, report_desc_len, &summary))
        return nullptr;

    hid_report_descriptor_t *report_descriptor = hid_descriptor_new(&summary);
    if (!report_descriptor)
        return nullptr;

    hid_parser_state_t parser = {0};
    hid_global_state_t *global_state = &parser.global_states_stack[0];
    hid_local_state_t *local_state = &parser.local_state;
    int8_t active_collection_idx = -1;


    size_t position = 0;
    hid_raw_short_item_t item;

    item_parse_result_t item_parse_result;
    while ((item_parse_result = try_parse_next_item(report_desc, report_desc_len, &position, &item))) {
        if (item_parse_result == HID_ITEM_PARSE_ERROR) {
            goto error;;
        }
        switch (item.item_type) {
            case SHORT_ITEM_TYPE_MAIN: {
                switch (item.tag) {
                    case HID_REPORT_MAIN_ITEM_OUTPUT:
                    case HID_REPORT_MAIN_ITEM_FEATURE:
                    case HID_REPORT_MAIN_ITEM_INPUT: {
                        const hid_report_main_item_flags_t *flags = (hid_report_main_item_flags_t *) &item.udata;
                        const uint8_t report_id = global_state->report_id;
                        const hid_report_type_t report_type = get_report_type_from_tag(item.tag);

                        if (HID_MAIN_ITEM_IS_CONSTANT(flags)) {
                            hid_field_t *field = reserve_field(report_descriptor, report_id, report_type);
                            if (field == nullptr) {
                                goto error;
                            }
                            field->collection_idx = active_collection_idx;
                            field->bit_size = global_state->report_size * global_state->report_count;
                            field->flags.flags = item.udata;
                        } else {
                            if (HID_MAIN_ITEM_IS_VARIABLE(flags)) {
                                for (int32_t f = 0; f < global_state->report_count; f++) {
                                    uint16_t usage = 0;
                                    uint16_t usage_page = global_state->usage_page;
                                    if (local_state->use_range) {
                                        usage = min16(local_state->usage_min.usage + f, local_state->usage_max.usage);
                                        if (local_state->usage_min.is_extended_usage) {
                                            usage_page = local_state->usage_min.usage_page;
                                        }
                                    } else {
                                        hid_parsing_usage_t parsing_usage = local_state->usages[min16(
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
                                    field->flags.flags = item.udata;
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
                                    field->flags.flags = item.udata;
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
                        collection->type = item.udata;
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
                switch (item.tag) {
                    case HID_REPORT_GLOBAL_ITEM_USAGE_PAGE: {
                        global_state->usage_page = item.udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_LOGICAL_MIN: {
                        global_state->logical.min = item.sdata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_LOGICAL_MAX: {
                        global_state->logical.max = item.sdata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_PHYSICAL_MIN: {
                        global_state->physical.min = item.sdata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_PHYSICAL_MAX: {
                        global_state->physical.max = item.sdata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_UNIT_EXPONENT: {
                        const int32_t code = item.sdata & 0xf;
                        if (code >= 0x8) {
                            global_state->unit_exponent = -16 + code;
                        } else {
                            global_state->unit_exponent = code;
                        }
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_UNIT: {
                        global_state->unit = item.udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_REPORT_SIZE: {
                        global_state->report_size = item.udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_REPORT_ID: {
                        global_state->report_id = item.udata;
                        break;
                    }
                    case HID_REPORT_GLOBAL_ITEM_REPORT_COUNT: {
                        global_state->report_count = item.udata;
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
                switch (item.tag) {
                    case HID_REPORT_LOCAL_ITEM_USAGE: {
                        if (local_state->usage_count > MAX_USAGES)
                            goto error;
                        local_state->use_range = false;
                        parse_usage(&local_state->usages[local_state->usage_count], &item);
                        local_state->usage_count++;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_USAGE_MIN: {
                        local_state->use_range = true;
                        parse_usage(&local_state->usage_min, &item);
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_USAGE_MAX: {
                        local_state->use_range = true;
                        parse_usage(&local_state->usage_max, &item);
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DESIGNATOR_INDEX: {
                        local_state->designator_index = item.sdata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DESIGNATOR_MIN: {
                        local_state->designator_min = item.sdata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DESIGNATOR_MAX: {
                        local_state->designator_max = item.sdata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_STRING_INDEX: {
                        local_state->string_index = item.sdata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_STRING_MIN: {
                        local_state->string_min = item.sdata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_STRING_MAX: {
                        local_state->string_max = item.sdata;
                        break;
                    }
                    case HID_REPORT_LOCAL_ITEM_DELIMITER: {
                        local_state->delimiter = item.sdata;
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
    }

    return report_descriptor;
error:
    hid_report_descriptor_free(report_descriptor);
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

/**
 * Print the report descriptor, mainly used for the tests.
 */
void hid_report_descriptor_print(
    const hid_report_descriptor_t *report_descriptor,
    int (*print)(void *user_data, const char *format, ...) __attribute__((format(printf, 2, 3))),
    void *user_data
) {
    print(user_data, "Collections:\n");
    for (uint32_t c = 0; c < report_descriptor->collection_count; c++) {
        const hid_collection_t *collection = &report_descriptor->collections[c];
        print(user_data, "  [%" PRIu32 "] %s", c, collection_type_to_string(collection->type));
        print(user_data, ", UsagePage: %" PRIu16, collection->usage_page);
        print(user_data, ", Usage: %" PRIu16, collection->usage);
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
                print(user_data, ", Usage: %" PRId16 "-%" PRId16, field->usage.range.min, field->usage.range.max);
            } else {
                print(user_data, ", Usage: %" PRId16, field->usage.value);
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
