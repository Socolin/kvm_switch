#pragma once

#include <stdint.h>

#include "hid_manager.h"


void node_link_ctrl_init();

void node_link_ctrl_task();

// ╔══════════════════════════════════╗
// ║     Enqueue message to send      ║
// ╚══════════════════════════════════╝

void node_link_ctrl_enqueue_send_init(
    uint8_t computer_id
);

void node_link_ctrl_enqueue_broadcast_hid_mount(
    const hid_t *hid
);

void node_link_ctrl_enqueue_send_hid_mount(
    uint8_t computer_id,
    const hid_t *hid
);

void node_link_ctrl_enqueue_broadcast_hid_umount(
    uint8_t dev_addr,
    uint8_t host_hid_idx
);

void node_link_ctrl_enqueue_send_start_usb_device(
    uint8_t computer_id,
    uint16_t vid,
    uint16_t pid
);

void node_link_ctrl_enqueue_broadcast_start_usb_device(
    uint16_t vid,
    uint16_t pid
);

void node_link_ctrl_enqueue_send_report(
    uint8_t computer_id,
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    const uint8_t *report_data,
    uint8_t report_data_length
);
