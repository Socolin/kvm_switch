#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "unity.h"
#include "report_descriptor.h"
#include "../test_utils/string_diff.h"
#include "../test_utils/test_utils_string.h"
#include "../test_utils/test_utils_file.h"

void setUp() {
}

void tearDown() {
}

void is_report_id_present_in_descriptor__should_return_false_for_null_descriptor() {
    TEST_ASSERT_FALSE(is_report_id_present_in_descriptor(nullptr,0));
}

static int parse_hex_char(int c) {
    if (isalpha(c)) {
        if (isupper(c))
            return c - 'A' + 10;
        return c - 'a' + 10;
    }
    if (isdigit(c))
        return c - '0';
    return -1;
}

static bool load_hid_file(
    const char *name,
    uint8_t **descriptor_data,
    uint16_t *descriptor_data_size
) {
    char hid_filepath[512] = {};
    string_combine(hid_filepath, TEST_SRC_DIR, "/shared/report_descriptors/", name, ".hid.txt");
    char *test_file_content = load_file_in_memory(hid_filepath);
    const size_t test_file_size = strlen(test_file_content);

    uint8_t *descriptor = calloc(4096, 1);
    size_t descriptor_len = 0;
    int i = 0;
    while (i < test_file_size) {
        const char c = test_file_content[i++];
        if (c == '#') {
            while (i < test_file_size && test_file_content[i] != '\n')
                i++;
            continue;
        }

        if (c == ' ' && i >= 3) {
            const int c1 = parse_hex_char(test_file_content[i - 3]);
            const int c2 = parse_hex_char(test_file_content[i - 2]);
            if (c1 == -1 || c2 == -1)
                continue;
            const uint8_t b = ((c1 << 4) & 0xf0) | (c2 & 0x0f);
            if (descriptor_len >= 4096)
                break;
            descriptor[descriptor_len++] = b;
        }
    }
    free(test_file_content);
    *descriptor_data = descriptor;
    *descriptor_data_size = descriptor_len;
    return true;
}

void parse_report_descriptor__should_parse_examples(
    const char *name
) {
    char expected_filepath[512] = {};
    string_combine(expected_filepath, TEST_SRC_DIR, "/shared/report_descriptors/", name, ".expected.txt");
    char *expected = load_file_in_memory(expected_filepath);

    uint8_t *descriptor = nullptr;
    uint16_t descriptor_len = 0;
    TEST_ASSERT_TRUE(load_hid_file(name, &descriptor, &descriptor_len));

    hid_descriptor_t *actual = parse_report_descriptor(descriptor, descriptor_len);
    free(descriptor);
    TEST_ASSERT_NOT_NULL(actual);

    test_print_buffer_t test_buffer = {0};
    print_report_descriptor(actual, print_to_buffer, &test_buffer);

    char actual_filepath[512] = {};
    string_combine(actual_filepath, TEST_SRC_DIR, "/shared/report_descriptors/", name, ".actual.txt");
    FILE *actual_file = fopen(actual_filepath, "w");
    TEST_ASSERT_NOT_NULL(actual_file);
    fwrite(test_buffer.buffer, sizeof(char), test_buffer.length, actual_file);
    fclose(actual_file);

    TEST_ASSERT_EQUAL_STRING_DIFF(expected, test_buffer.buffer);
    free(expected);
    free(actual);
}

void parse_report_descriptor__should_parse_examples_keyboard() {
    parse_report_descriptor__should_parse_examples("keyboard");
}

/** Sign extended Logical Minimum, and restoring the parent collection on End Collection. */
void parse_report_descriptor__should_parse_examples_mouse() {
    parse_report_descriptor__should_parse_examples("mouse");
}

/** Push saves a copy of the global item state, Pop restores it. */
void parse_report_descriptor__should_parse_examples_push_pop() {
    parse_report_descriptor__should_parse_examples("push_pop");
}

/** Report Count larger than the declared Usage list repeats the last Usage. */
void parse_report_descriptor__should_parse_examples_usage_list() {
    parse_report_descriptor__should_parse_examples("usage_list");
}

/** More declared Usages than MAX_USAGES used to hold. */
void parse_report_descriptor__should_parse_examples_many_usages() {
    parse_report_descriptor__should_parse_examples("many_usages");
}

/** 4 byte Usage items carry their own Usage Page. */
void parse_report_descriptor__should_parse_examples_usage_page_qualified() {
    parse_report_descriptor__should_parse_examples("usage_page_qualified");
}

/** Fields are grouped per (Report ID, report type). */
void parse_report_descriptor__should_parse_examples_report_ids() {
    parse_report_descriptor__should_parse_examples("report_ids");
}

/** Physical range and the 4 bit two's complement Unit Exponent. */
void parse_report_descriptor__should_parse_examples_dial() {
    parse_report_descriptor__should_parse_examples("dial");
}

// Cases that cannot be expressed through print_report_descriptor, because they
// are about the parser refusing input rather than about what it produces.

/**
 * A descriptor that fails to parse must not leave any state behind that breaks
 * the next call: this one opens a collection and then hits an invalid Main tag,
 * so it never reaches its End Collection.
 */
void parse_report_descriptor__should_not_leak_state_into_the_next_parse() {
    const uint8_t unbalanced[] = {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0x09, 0x02, // Usage (Mouse)
        0xA1, 0x01, // Collection (Application)
        0x01, 0x00, // Main item, reserved tag 0 -> rejected
    };
    const uint8_t valid[] = {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0x09, 0x02, // Usage (Mouse)
        0xA1, 0x01, // Collection (Application)
        0x09, 0x30, // Usage (X)
        0x75, 0x08, // Report Size (8)
        0x95, 0x01, // Report Count (1)
        0x81, 0x06, // Input (Data, Variable, Relative)
        0xC0, // End Collection
    };

    for (int attempt = 0; attempt < 8; attempt++) {
        TEST_ASSERT_NULL(parse_report_descriptor(unbalanced, sizeof(unbalanced)));

        hid_descriptor_t *actual = parse_report_descriptor(valid, sizeof(valid));
        TEST_ASSERT_NOT_NULL(actual);
        TEST_ASSERT_EQUAL(1, actual->collection_count);
        TEST_ASSERT_EQUAL(1, actual->field_count);
        TEST_ASSERT_EQUAL(0, actual->fields[0].collection_idx);
        free(actual);
    }
}

/** Asking for more fields than MAX_FIELDS must fail cleanly, not overflow. */
void parse_report_descriptor__should_fail_when_report_count_exceeds_max_fields() {
    const uint8_t descriptor[] = {
        0x05, 0x09, // Usage Page (Button)
        0x19, 0x01, // Usage Minimum (1)
        0x29, 0xFF, // Usage Maximum (255)
        0x15, 0x00, // Logical Minimum (0)
        0x25, 0x01, // Logical Maximum (1)
        0x75, 0x01, // Report Size (1)
        0x95, 0xFF, // Report Count (255), MAX_FIELDS is 64
        0x81, 0x02, // Input (Data, Variable, Absolute)
    };
    TEST_ASSERT_NULL(parse_report_descriptor(descriptor, sizeof(descriptor)));
}

/** An item whose data runs past the end of the descriptor must be rejected. */
void parse_report_descriptor__should_fail_on_truncated_item() {
    const uint8_t descriptor[] = {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0x26, 0xFF, // Logical Maximum, 2 bytes announced but only 1 present
    };
    TEST_ASSERT_NULL(parse_report_descriptor(descriptor, sizeof(descriptor)));
}

/** More nesting than COLLECTION_STACK_SIZE must fail cleanly. */
void parse_report_descriptor__should_fail_on_collection_nested_too_deep() {
    const uint8_t descriptor[] = {
        0xA1, 0x01, // Collection (Application)
        0xA1, 0x00, // Collection (Physical)
        0xA1, 0x02, // Collection (Logical)
        0xA1, 0x02, // Collection (Logical)
        0xA1, 0x02, // Collection (Logical)
    };
    TEST_ASSERT_NULL(parse_report_descriptor(descriptor, sizeof(descriptor)));
}

/** End Collection without a matching Collection must fail cleanly. */
void parse_report_descriptor__should_fail_on_unmatched_end_collection() {
    const uint8_t descriptor[] = {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0xC0, // End Collection
    };
    TEST_ASSERT_NULL(parse_report_descriptor(descriptor, sizeof(descriptor)));
}

void is_report_id_present_in_descriptor__should_return_true_when_report_id_is_declared() {
    const uint8_t descriptor[] = {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0x09, 0x06, // Usage (Keyboard)
        0xA1, 0x01, // Collection (Application)
        0x85, 0x03, // Report ID (3)
        0xC0, // End Collection
    };
    TEST_ASSERT_TRUE(is_report_id_present_in_descriptor(descriptor, sizeof(descriptor)));
}

void is_report_id_present_in_descriptor__should_return_false_when_no_report_id_is_declared() {
    const uint8_t descriptor[] = {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0x09, 0x06, // Usage (Keyboard)
        0xA1, 0x01, // Collection (Application)
        0x75, 0x08, // Report Size (8), the 8 is data and must not be read as a Report ID
        0xC0, // End Collection
    };
    TEST_ASSERT_FALSE(is_report_id_present_in_descriptor(descriptor, sizeof(descriptor)));
}

/**
 * A Long item's payload must be skipped, not walked into: the 0x85 byte inside
 * it is data, not a Report ID.
 */
void is_report_id_present_in_descriptor__should_skip_long_item_payload() {
    const uint8_t descriptor[] = {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0xFE, 0x02, 0x0F, // Long item, 2 data bytes, tag 0x0F
        0x85, 0x03, //   payload that looks like Report ID (3)
        0xC0, // End Collection
    };
    TEST_ASSERT_FALSE(is_report_id_present_in_descriptor(descriptor, sizeof(descriptor)));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(is_report_id_present_in_descriptor__should_return_false_for_null_descriptor);
    RUN_TEST(is_report_id_present_in_descriptor__should_return_true_when_report_id_is_declared);
    RUN_TEST(is_report_id_present_in_descriptor__should_return_false_when_no_report_id_is_declared);
    RUN_TEST(is_report_id_present_in_descriptor__should_skip_long_item_payload);

    RUN_TEST(parse_report_descriptor__should_parse_examples_keyboard);
    RUN_TEST(parse_report_descriptor__should_parse_examples_mouse);
    RUN_TEST(parse_report_descriptor__should_parse_examples_push_pop);
    RUN_TEST(parse_report_descriptor__should_parse_examples_usage_list);
    RUN_TEST(parse_report_descriptor__should_parse_examples_many_usages);
    RUN_TEST(parse_report_descriptor__should_parse_examples_usage_page_qualified);
    RUN_TEST(parse_report_descriptor__should_parse_examples_report_ids);
    RUN_TEST(parse_report_descriptor__should_parse_examples_dial);

    RUN_TEST(parse_report_descriptor__should_not_leak_state_into_the_next_parse);
    RUN_TEST(parse_report_descriptor__should_fail_when_report_count_exceeds_max_fields);
    RUN_TEST(parse_report_descriptor__should_fail_on_truncated_item);
    RUN_TEST(parse_report_descriptor__should_fail_on_collection_nested_too_deep);
    RUN_TEST(parse_report_descriptor__should_fail_on_unmatched_end_collection);
    return UNITY_END();
}
