#include <string.h>

#include "unity.h"
#include "hid_keyboard_report_util.h"
#include "hid_report_descriptor.h"

#define KEY_A 0x04
#define KEY_B 0x05
#define KEY_C 0x06
#define KEY_D 0x07
#define KEY_E 0x08
#define KEY_F 0x09
#define KEY_G 0x0A
#define KEY_LEFT_CTRL 0xE0
#define KEY_LEFT_SHIFT 0xE1
#define KEY_LEFT_ALT 0xE2
#define KEY_RIGHT_GUI 0xE7

void setUp() {
}

void tearDown() {
}

/**
 * Standard boot style keyboard: 8 modifier bits, 1 reserved byte, then a 6 key
 * array. Payload is 8 bytes and uses no Report ID.
 */
static const uint8_t keyboard_descriptor[] = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection (Application)
    0x05, 0x07, //   Usage Page (Key Codes)
    0x19, 0xE0, //   Usage Minimum (224)
    0x29, 0xE7, //   Usage Maximum (231)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x02, //   Input (Data, Variable, Absolute)
    0x95, 0x01, //   Report Count (1)
    0x75, 0x08, //   Report Size (8)
    0x81, 0x01, //   Input (Constant), the reserved byte
    0x95, 0x06, //   Report Count (6)
    0x75, 0x08, //   Report Size (8)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x65, //   Logical Maximum (101)
    0x05, 0x07, //   Usage Page (Key Codes)
    0x19, 0x00, //   Usage Minimum (0)
    0x29, 0x65, //   Usage Maximum (101)
    0x81, 0x00, //   Input (Data, Array)
    0xC0, // End Collection
};

/** A device that only declares Generic Desktop fields, so no keycode at all. */
static const uint8_t mouse_descriptor[] = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)
    0x05, 0x09, //     Usage Page (Button)
    0x19, 0x01, //     Usage Minimum (1)
    0x29, 0x03, //     Usage Maximum (3)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x75, 0x01, //     Report Size (1)
    0x95, 0x03, //     Report Count (3)
    0x81, 0x02, //     Input (Data, Variable, Absolute)
    0x75, 0x05, //     Report Size (5)
    0x95, 0x01, //     Report Count (1)
    0x81, 0x01, //     Input (Constant), padding
    0x05, 0x01, //     Usage Page (Generic Desktop)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x02, //     Report Count (2)
    0x81, 0x06, //     Input (Data, Variable, Relative)
    0xC0, //   End Collection
    0xC0, // End Collection
};

/**
 * Two input reports: Report ID 1 carries the 8 modifier bits, Report ID 2
 * carries a 2 key array. The Report ID byte itself is stripped by the caller,
 * so both payloads start at bit 0.
 */
static const uint8_t report_ids_descriptor[] = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection (Application)
    0x85, 0x01, //   Report ID (1)
    0x05, 0x07, //   Usage Page (Key Codes)
    0x19, 0xE0, //   Usage Minimum (224)
    0x29, 0xE7, //   Usage Maximum (231)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x02, //   Input (Data, Variable, Absolute)
    0x85, 0x02, //   Report ID (2)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x65, //   Logical Maximum (101)
    0x19, 0x00, //   Usage Minimum (0)
    0x29, 0x65, //   Usage Maximum (101)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x02, //   Report Count (2)
    0x81, 0x00, //   Input (Data, Array)
    0xC0, // End Collection
};

static hid_report_descriptor_t *parse(
    const uint8_t *descriptor,
    const uint16_t descriptor_len
) {
    hid_report_descriptor_t *report_descriptor = hid_report_descriptor_parse(descriptor, descriptor_len);
    TEST_ASSERT_NOT_NULL(report_descriptor);
    return report_descriptor;
}

void hid_report_get_pressed_keys__should_return_no_key_for_an_empty_report() {
    hid_report_descriptor_t *report_descriptor = parse(keyboard_descriptor, sizeof(keyboard_descriptor));
    const uint8_t report[8] = {0};
    uint8_t pressed_keys[16] = {0};

    const size_t count = hid_report_get_pressed_keys(
        report_descriptor, report, sizeof(report), 0, pressed_keys, sizeof(pressed_keys));

    TEST_ASSERT_EQUAL_size_t(0, count);
    hid_report_descriptor_free(report_descriptor);
}

void hid_report_get_pressed_keys__should_report_keys_from_the_key_array() {
    hid_report_descriptor_t *report_descriptor = parse(keyboard_descriptor, sizeof(keyboard_descriptor));
    const uint8_t report[8] = {0x00, 0x00, KEY_A, KEY_B, 0x00, 0x00, 0x00, 0x00};
    uint8_t pressed_keys[16] = {0};

    const size_t count = hid_report_get_pressed_keys(
        report_descriptor, report, sizeof(report), 0, pressed_keys, sizeof(pressed_keys));

    const uint8_t expected[] = {KEY_A, KEY_B};
    TEST_ASSERT_EQUAL_size_t(2, count);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, pressed_keys, 2);
    hid_report_descriptor_free(report_descriptor);
}

/** Each modifier is a 1 bit field, so the bit offset inside the byte matters. */
void hid_report_get_pressed_keys__should_report_modifier_keys() {
    hid_report_descriptor_t *report_descriptor = parse(keyboard_descriptor, sizeof(keyboard_descriptor));
    const uint8_t report[8] = {0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t pressed_keys[16] = {0};

    const size_t count = hid_report_get_pressed_keys(
        report_descriptor, report, sizeof(report), 0, pressed_keys, sizeof(pressed_keys));

    const uint8_t expected[] = {KEY_LEFT_SHIFT, KEY_LEFT_ALT, KEY_RIGHT_GUI};
    TEST_ASSERT_EQUAL_size_t(3, count);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, pressed_keys, 3);
    hid_report_descriptor_free(report_descriptor);
}

void hid_report_get_pressed_keys__should_report_modifier_and_array_keys_together() {
    hid_report_descriptor_t *report_descriptor = parse(keyboard_descriptor, sizeof(keyboard_descriptor));
    const uint8_t report[8] = {0x01, 0x00, KEY_C, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t pressed_keys[16] = {0};

    const size_t count = hid_report_get_pressed_keys(
        report_descriptor, report, sizeof(report), 0, pressed_keys, sizeof(pressed_keys));

    const uint8_t expected[] = {KEY_LEFT_CTRL, KEY_C};
    TEST_ASSERT_EQUAL_size_t(2, count);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, pressed_keys, 2);
    hid_report_descriptor_free(report_descriptor);
}

/**
 * The reserved byte is a Constant field: it must be skipped, but it still
 * takes 8 bits in the report.
 */
void hid_report_get_pressed_keys__should_ignore_the_constant_reserved_byte() {
    hid_report_descriptor_t *report_descriptor = parse(keyboard_descriptor, sizeof(keyboard_descriptor));
    const uint8_t report[8] = {0x00, 0xFF, KEY_A, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t pressed_keys[16] = {0};

    const size_t count = hid_report_get_pressed_keys(
        report_descriptor, report, sizeof(report), 0, pressed_keys, sizeof(pressed_keys));

    const uint8_t expected[] = {KEY_A};
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, pressed_keys, 1);
    hid_report_descriptor_free(report_descriptor);
}

/** A device without any keycode field never reports a pressed key. */
void hid_report_get_pressed_keys__should_ignore_non_keycode_fields() {
    hid_report_descriptor_t *report_descriptor = parse(mouse_descriptor, sizeof(mouse_descriptor));
    const uint8_t report[3] = {0x01, 0x10, 0xF0}; // button 1 down, X +16, Y -16
    uint8_t pressed_keys[16] = {0};

    const size_t count = hid_report_get_pressed_keys(
        report_descriptor, report, sizeof(report), 0, pressed_keys, sizeof(pressed_keys));

    TEST_ASSERT_EQUAL_size_t(0, count);
    hid_report_descriptor_free(report_descriptor);
}

void hid_report_get_pressed_keys__should_only_read_the_fields_of_the_requested_report_id() {
    hid_report_descriptor_t *report_descriptor = parse(report_ids_descriptor, sizeof(report_ids_descriptor));
    const uint8_t modifiers_report[1] = {0x01};
    const uint8_t keys_report[2] = {KEY_D, KEY_E};
    uint8_t pressed_keys[16] = {0};

    size_t count = hid_report_get_pressed_keys(
        report_descriptor, modifiers_report, sizeof(modifiers_report), 1, pressed_keys, sizeof(pressed_keys));
    const uint8_t expected_modifiers[] = {KEY_LEFT_CTRL};
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_modifiers, pressed_keys, 1);

    memset(pressed_keys, 0, sizeof(pressed_keys));
    count = hid_report_get_pressed_keys(
        report_descriptor, keys_report, sizeof(keys_report), 2, pressed_keys, sizeof(pressed_keys));
    const uint8_t expected_keys[] = {KEY_D, KEY_E};
    TEST_ASSERT_EQUAL_size_t(2, count);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_keys, pressed_keys, 2);

    hid_report_descriptor_free(report_descriptor);
}

void hid_report_get_pressed_keys__should_return_no_key_for_an_unknown_report_id() {
    hid_report_descriptor_t *report_descriptor = parse(report_ids_descriptor, sizeof(report_ids_descriptor));
    const uint8_t report[2] = {KEY_D, KEY_E};
    uint8_t pressed_keys[16] = {0};

    const size_t count = hid_report_get_pressed_keys(
        report_descriptor, report, sizeof(report), 42, pressed_keys, sizeof(pressed_keys));

    TEST_ASSERT_EQUAL_size_t(0, count);
    hid_report_descriptor_free(report_descriptor);
}

/** More keys pressed than the output buffer holds must not overflow it. */
void hid_report_get_pressed_keys__should_not_write_past_the_output_buffer() {
    hid_report_descriptor_t *report_descriptor = parse(keyboard_descriptor, sizeof(keyboard_descriptor));
    const uint8_t report[8] = {0x03, 0x00, KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F};
    uint8_t pressed_keys[8] = {0};
    const size_t pressed_keys_len = 3;

    const size_t count = hid_report_get_pressed_keys(
        report_descriptor, report, sizeof(report), 0, pressed_keys, pressed_keys_len);

    const uint8_t expected[] = {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_A};
    TEST_ASSERT_EQUAL_size_t(pressed_keys_len, count);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, pressed_keys, pressed_keys_len);
    for (size_t i = pressed_keys_len; i < sizeof(pressed_keys); i++) {
        TEST_ASSERT_EQUAL_UINT8(0, pressed_keys[i]);
    }
    hid_report_descriptor_free(report_descriptor);
}

/** A report shorter than the descriptor announces must not be read past its end. */
void hid_report_get_pressed_keys__should_stop_at_the_end_of_a_truncated_report() {
    hid_report_descriptor_t *report_descriptor = parse(keyboard_descriptor, sizeof(keyboard_descriptor));
    const uint8_t report[3] = {0x00, 0x00, KEY_G};
    uint8_t pressed_keys[16] = {0};

    const size_t count = hid_report_get_pressed_keys(
        report_descriptor, report, sizeof(report), 0, pressed_keys, sizeof(pressed_keys));

    const uint8_t expected[] = {KEY_G};
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, pressed_keys, 1);
    hid_report_descriptor_free(report_descriptor);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(hid_report_get_pressed_keys__should_return_no_key_for_an_empty_report);
    RUN_TEST(hid_report_get_pressed_keys__should_report_keys_from_the_key_array);
    RUN_TEST(hid_report_get_pressed_keys__should_report_modifier_keys);
    RUN_TEST(hid_report_get_pressed_keys__should_report_modifier_and_array_keys_together);
    RUN_TEST(hid_report_get_pressed_keys__should_ignore_the_constant_reserved_byte);
    RUN_TEST(hid_report_get_pressed_keys__should_ignore_non_keycode_fields);
    RUN_TEST(hid_report_get_pressed_keys__should_only_read_the_fields_of_the_requested_report_id);
    RUN_TEST(hid_report_get_pressed_keys__should_return_no_key_for_an_unknown_report_id);
    RUN_TEST(hid_report_get_pressed_keys__should_not_write_past_the_output_buffer);
    RUN_TEST(hid_report_get_pressed_keys__should_stop_at_the_end_of_a_truncated_report);
    return UNITY_END();
}
