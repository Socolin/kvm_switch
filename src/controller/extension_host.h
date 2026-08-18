#pragma once

#include <stdint.h>


void extension_host_init();

void extension_host_task();

void extension_host_broadcast_hid_mount(
    uint8_t report_id,
    uint8_t* report_data,
    uint8_t report_data_length
);

void extension_host_broadcast_hid_umount(
    uint8_t kvm_hid_idx
);

void extension_host_broadcast_start_usb_device();

void extension_host_send_report(
    uint8_t computer_id,
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    uint8_t* report_data,
    uint8_t report_data_length
);

