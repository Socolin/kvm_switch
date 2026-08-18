#pragma once

#include <stdint.h>

void usb_device_init(
    uint8_t computer_id,
    uint16_t vid,
    uint16_t pid
);

void usb_device_task();

void usb_device_connect_to_computer();

void usb_device_send_report(
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    const uint8_t* report_data,
    uint8_t report_data_len
);