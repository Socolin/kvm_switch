#pragma once

#include <stdint.h>

typedef void (*set_report_cb_t)(
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    uint8_t report_type,
    uint8_t const *report_data,
    uint16_t report_data_len
);

typedef void (*set_hid_protocol_cb_t)(
    uint8_t kvm_hid_idx,
    uint8_t hid_protocol
);

typedef void (*usb_device_mounted_cb_t)();

typedef void (*usb_device_unmounted_cb_t)();

void usb_device_init(
    set_report_cb_t set_report_cb,
    set_hid_protocol_cb_t set_hid_protocol_cb,
    usb_device_mounted_cb_t usb_device_mounted_cb,
    usb_device_unmounted_cb_t usb_device_unmounted_cb
);

void usb_device_task();

/**
 * Initialize the USB device, after this, the kvm will be visible on the computer.
 * This support re-init if the hid config changes.
 */
void usb_device_connect_to_computer(
    uint8_t rhport,
    uint16_t vid,
    uint16_t pid
);

/**
 * Send a report to the host. When a key is pressed on a keyboard or when the mouse moves etc...
 */
void usb_device_send_report(
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    const uint8_t *report_data,
    uint16_t report_data_len
);
