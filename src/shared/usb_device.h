#pragma once

#include <stdint.h>

typedef void (*set_report_cb_t)(
    uint8_t computer_id,
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    uint8_t report_type,
    uint8_t const *report_data,
    uint16_t report_data_len
);

typedef void (*set_hid_protocol_cb_t)(
    uint8_t computer_id,
    uint8_t kvm_hid_idx,
    uint8_t hid_protocol
);

void usb_device_init(
    uint8_t computer_id,
    uint16_t vid,
    uint16_t pid,
    set_report_cb_t set_report_cb,
    set_hid_protocol_cb_t set_hid_protocol_cb
);

void usb_device_task();

void usb_device_connect_to_computer(
    uint8_t rhport
);

void usb_device_send_report(
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    const uint8_t *report_data,
    uint8_t report_data_len
);

