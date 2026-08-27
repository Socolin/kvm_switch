#pragma once

#include <stddef.h>
#include <stdint.h>

// ╔══════════════════════════════════╗
// ║                Core              ║
// ╚══════════════════════════════════╝

void usb_host_init();

void usb_host_task();

// ╔══════════════════════════════════╗
// ║             HID actions          ║
// ╚══════════════════════════════════╝

typedef enum {
    HID_SET_REPORT,
    HID_SET_PROTOCOL,
} hid_action_opcode_t;

typedef struct {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
    const uint8_t report_id; /**< HID report ID */
    const uint8_t report_type; /**< \see hid_report_type_t */
    const uint16_t buffer_len; /**< Length of the report data */
    uint8_t buffer[96];
} hid_action_set_report_t;

typedef struct {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
    const uint8_t hid_protocol; /**< The HID protocol (boot / report) \see hid_protocol_mode_enum_t */
} hid_action_set_protocol_t;

typedef struct {
    hid_action_opcode_t opcode;
    size_t data_len;
    uint8_t data[128];
} hid_action_t;

/**
 * @param dev_addr Device address (Which port the device is connected. Values: 1, 2)
 * @param host_hid_idx HID interface index on USB Host side (Where the keyboard / mouse are connected)
 * @param hid_protocol The HID protocol (boot / report) \see hid_protocol_mode_enum_t
 * @return
 *   - **true**: if the message was enqueued successfully
 *   - **false**: otherwise
 */
bool usb_host_enqueue_set_protocol(
    uint8_t dev_addr,
    uint8_t host_hid_idx,
    uint8_t hid_protocol
);

/**
 * Mainly used to update the keyboard LEDs (NUM LOCK, CAPS LOCK, ...)
 * @param dev_addr Device address (Which port the device is connected. Values: 1, 2)
 * @param host_hid_idx HID interface index on USB Host side (Where the keyboard / mouse are connected)
 * @param report_id HID report ID
 * @param report_type \see hid_report_type_t
 * @param report_data The HID report data
 * @param report_data_len The HID report data length
 * @return
 *   - **true**: if the message was enqueued successfully
 *   - **false**: otherwise
 */
bool usb_host_enqueue_set_report(
    uint8_t dev_addr,
    uint8_t host_hid_idx,
    uint8_t report_id,
    uint8_t report_type,
    uint8_t const *report_data,
    uint16_t report_data_len
);
