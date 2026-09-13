#pragma once
#include <stdint.h>

#include "kvm_switch_config.h"
#include "tusb_config.h"
#include "version.h"

#define WEB_USB_PROTOCOL_VERSION 1
#define IN_COMMAND(x) x
#define OUT_COMMAND(x) (x | 0x80)
#define IS_IN_COMMAND(x) (((x) & 0x80) == 0)
#define IS_OUT_COMMAND(x) (((x) & 0x80) != 0)

enum {
    COMMAND_IN_OP_GET_INFO = IN_COMMAND(0x01),
    COMMAND_IN_OP_GET_COMPUTER_STATE = IN_COMMAND(0x02),
    COMMAND_IN_OP_GET_HID_DEVICE = IN_COMMAND(0x03),
    COMMAND_IN_OP_GET_HID_STATE = IN_COMMAND(0x04),
    COMMAND_IN_OP_GET_HID_DESCRIPTOR = IN_COMMAND(0x05),
    COMMAND_IN_OP_GET_GENERAL_CONFIG = IN_COMMAND(0x06),
    COMMAND_OUT_OP_SET_GENERAL_CONFIG = OUT_COMMAND(0x07),
    COMMAND_IN_OP_GET_KEYBOARD_SHORTCUTS = IN_COMMAND(0x08),
    COMMAND_IN_OP_GET_KEYBOARD_SHORTCUT = IN_COMMAND(0x09),
    COMMAND_OUT_OP_SET_SHORTCUT = OUT_COMMAND(0x0A),
    COMMAND_IN_OP_GET_LOGS = IN_COMMAND(0x0B),
};

typedef struct __attribute__((packed)) {
    uint16_t protocol_version;
    uint8_t computer_count;
    uint8_t hid_interface_count;
    uint8_t hid_device_count;
    uint64_t current_time;
    // uint8_t version_len;
    // unit8_t *version;
    // uint8_t build_date_len;
    // unit8_t *build_date;
} web_usb_cmd_get_info_command_data_t;

typedef struct __attribute__((packed)) {
    uint8_t computer_id;
    uint8_t state;
    uint8_t hid_count;
    uint8_t hid_protocol_per_interface[CFG_TUH_HID];
} web_usb_cmd_get_computer_state_data_t;

typedef struct __attribute__((packed)) {
    uint8_t dev_addr;
    bool is_mounted;
} web_usb_cmd_get_hid_device_data_t;

typedef struct __attribute__((packed)) {
    uint8_t enabled;
    uint8_t dev_addr;
    uint8_t host_hid_idx;
    uint8_t kvm_hid_idx;
    uint8_t itf_protocol;
    bool use_report_id;
    bool has_keyboard_report;
    uint16_t vid;
    uint16_t pid;
} web_usb_cmd_get_hid_state_data_t;

typedef struct __attribute__((packed)) {
    uint16_t vid;
    uint16_t pid;
} web_usb_cmd_get_general_config_data_t;

typedef struct __attribute__((packed)) {
    uint16_t vid;
    uint16_t pid;
} web_usb_cmd_set_general_config_data_t;

typedef struct __attribute__((packed)) {
    uint16_t keyboard_shortcut_count;
} web_usb_cmd_get_keyboard_shortcuts_command_data_t;

typedef struct __attribute__((packed)) {
    uint8_t shortcut_id;
    bool enabled;
    uint8_t action;
    // uint8_t key_count;
    // uint8_t keys[MAX_KEYS_PER_SHORTCUT];
    // uint8_t data_len;
    // uint8_t data[MAX_DATA_PER_SHORTCUT];
} web_usb_cmd_get_keyboard_shortcut_data_t;

typedef struct __attribute__((packed)) {
    uint8_t shortcut_id;
    bool enabled;
    uint8_t action;
    uint8_t key_count;
    uint8_t keys[MAX_KEYS_PER_SHORTCUT];
    uint8_t data_len;
    uint8_t data[MAX_DATA_PER_SHORTCUT];
} web_usb_cmd_set_keyboard_shortcut_data_t;
