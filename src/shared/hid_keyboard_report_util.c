#include "hid_keyboard_report_util.h"

#include "hid_usage.h"
#include "hid_report_descriptor.h"

#define LEFT_CTRL_KEY 0xE0

bool hid_report_descriptor_contains_keycodes(
    const hid_report_descriptor_t *report_descriptor
) {
    for (int i = 0; i < report_descriptor->report_count; ++i) {
        const hid_report_definition_t *report_def = &report_descriptor->reports_definitions[i];
        if (report_def->report_type != HID_DESCRIPTOR_REPORT_TYPE_INPUT)
            continue;
        for (size_t field_idx = 0; field_idx < report_def->field_count; ++field_idx) {
            const hid_field_t *hid_field = &report_descriptor->fields[report_def->field_indices[field_idx]];
            if (hid_field->usage_page == HID_USAGE_PAGE_KEYBOARD) {
                return true;
            }
        }
    }
    return false;
}

static void safe_add_pressed_key(
    uint8_t *pressed_keys,
    const size_t pressed_keys_len,
    const uint8_t key_id,
    size_t *pressed_keys_count
) {
    if (*pressed_keys_count >= pressed_keys_len)
        return;
    pressed_keys[*pressed_keys_count] = key_id;
    (*pressed_keys_count)++;
}

static uint32_t get_bits(
    const uint8_t *report,
    const size_t report_len,
    const size_t bit_offset,
    const size_t bit_size
) {
    const uint32_t report_len_bits = report_len * 8;
    if (bit_offset + bit_size > report_len_bits)
        return 0;

    uint32_t bits = 0;
    for (size_t bit_idx = 0; bit_idx < bit_size; bit_idx++) {
        const size_t bit_pos = bit_offset + bit_idx;
        bits |= ((report[bit_pos / 8] >> (bit_pos % 8)) & 1) << bit_idx;
    }
    return bits;
}

size_t hid_report_get_pressed_keys(
    const hid_report_descriptor_t *report_descriptor,
    const uint8_t *report,
    const uint16_t report_len,
    const uint8_t hid_report_id,
    uint8_t *pressed_keys,
    const size_t pressed_keys_len
) {
    size_t pressed_keys_count = 0;
    size_t bit_offset = 0;

    for (int i = 0; i < report_descriptor->report_count; ++i) {
        const hid_report_definition_t *report_def = &report_descriptor->reports_definitions[i];
        if (report_def->report_type != HID_DESCRIPTOR_REPORT_TYPE_INPUT)
            continue;
        if (report_def->report_id != hid_report_id)
            continue;
        for (size_t field_idx = 0; field_idx < report_def->field_count; ++field_idx) {
            const hid_field_t *hid_field = &report_descriptor->fields[report_def->field_indices[field_idx]];
            if (hid_field->usage_page == HID_USAGE_PAGE_KEYBOARD) {
                const uint32_t field_bits = get_bits(report, report_len, bit_offset, hid_field->bit_size);
                if (field_bits) {
                    if (hid_field->usage_kind == HID_FIELD_USAGE_KIND_RANGE) {
                        safe_add_pressed_key(pressed_keys, pressed_keys_len, hid_field->usage.range.min + field_bits, &pressed_keys_count);
                    } else if (hid_field->usage_kind == HID_FIELD_USAGE_KIND_VALUE){
                        safe_add_pressed_key(pressed_keys, pressed_keys_len, hid_field->usage.value, &pressed_keys_count);
                    } else if (hid_field->usage_kind == HID_FIELD_USAGE_KIND_ARRAY) {
                        const size_t usage_index = field_bits - hid_field->logical.min;
                        if (usage_index < hid_field->usage.array.value_count)
                            safe_add_pressed_key(pressed_keys, pressed_keys_len, hid_field->usage.array.values[usage_index], &pressed_keys_count);
                    }
                }
            }
            bit_offset += hid_field->bit_size;
        }
    }
    return pressed_keys_count;
}

/**
 * This one parse the standard HID keyboard report.
 * 1 byte modifier keys, 1 byte reserved, 6 bytes keys
 */
size_t hid_report_keyboard_boot_get_pressed_keys(
    const uint8_t *report,
    const uint16_t report_len,
    uint8_t *pressed_keys,
    const size_t pressed_keys_len
) {
    if (report_len != 8)
        return 0;

    size_t pressed_keys_count = 0;
    const uint8_t modifier_keys = report[0];
    for (uint8_t i = 0; i < 8; i++) {
        if (modifier_keys & (1 << i)) {
            safe_add_pressed_key(pressed_keys, pressed_keys_len, LEFT_CTRL_KEY + i, &pressed_keys_count);
        }
    }

    for (int i = 0; i < 6; i++) {
        const uint8_t key_id = report[i + 2];
        if (key_id) {
            safe_add_pressed_key(pressed_keys, pressed_keys_len, key_id, &pressed_keys_count);
        }
    }

    return pressed_keys_count;
}
