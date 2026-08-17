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
    const uint8_t dev_addr;
    const uint8_t itf_idx;
    const uint8_t report_id;
    const uint8_t report_type;
    const uint16_t buffer_len;
    uint8_t buffer[96];
} hid_action_set_report_t;

typedef struct {
    const uint8_t dev_addr;
    const uint8_t itf_idx;
    const uint8_t hid_protocol;
} hid_action_set_protocol_t;

typedef struct {
    hid_action_opcode_t opcode;
    size_t data_len;
    uint8_t data[128];
} hid_action_t;

bool usb_host_enqueue_set_protocol(
    uint8_t kvm_hid_idx,
    uint8_t hid_protocol
);

bool usb_host_enqueue_set_report(
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    uint8_t report_type,
    uint8_t const *report_data,
    uint16_t report_data_len
);