#ifndef HID_H
#define HID_H
#include <stdint.h>

#include "tusb.h"

// ╔══════════════════════════════════╗
// ║     Hid to computer messages     ║
// ╚══════════════════════════════════╝

typedef enum {
    HID_TO_COMPUTER_HID_MOUNT,
    HID_TO_COMPUTER_DEVICE_MOUNT,
    HID_TO_COMPUTER_HID_UMOUNT,
    HID_TO_COMPUTER_DEVICE_UMOUNT,
    HID_TO_COMPUTER_HID_REPORT,
} hid_to_computer_message_opcode_t;

typedef struct {
    hid_to_computer_message_opcode_t opcode;
    size_t data_len;
    uint8_t data[128];
} hid_to_computer_message_t;

typedef struct {
    const uint8_t dev_addr;
    const uint8_t itf_idx;
    const uint8_t itf_protocol;
    const uint16_t pid;
    const uint16_t vid;
    // Need to be malloc() / free();
    uint8_t *report_desc;
    const uint16_t desc_len;
} htc_message_hid_mount_data_t;

typedef struct {
    const uint8_t dev_addr;
} htc_message_device_mount_data_t;

typedef struct {
    const uint8_t dev_addr;
    const uint8_t idx;
} htc_message_hid_umount_data_t;

typedef struct {
    const uint8_t dev_addr;
} htc_message_device_umount_data_t;

typedef struct {
    const uint8_t dev_addr;
    const uint8_t idx;
    const uint8_t itf_protocol;
    const uint8_t hid_protocol;
    const uint8_t report_id;
    uint8_t report_data[96];
    const uint16_t report_data_len;
} htc_message_hid_report_data_t;

// ╔══════════════════════════════════╗
// ║     Computer to HID messages     ║
// ╚══════════════════════════════════╝

typedef enum {
    COMPUTER_TO_HID_SET_REPORT,
    COMPUTER_TO_HID_SET_HID_PROTOCOL,
} computer_to_hid_message_opcode_t;

typedef struct {
    const uint8_t dev_addr;
    const uint8_t itf_idx;
    const uint8_t report_id;
    const hid_report_type_t report_type;
    const uint16_t buffer_len;
    uint8_t buffer[96];
} cth_message_set_report_t;

typedef struct {
    const uint8_t dev_addr;
    const uint8_t itf_idx;
    const uint8_t hid_protocol;
} cth_message_set_hid_protocol_t;

typedef struct {
    computer_to_hid_message_opcode_t opcode;
    size_t data_len;
    uint8_t data[128];
} computer_to_hid_message_t;

#endif
