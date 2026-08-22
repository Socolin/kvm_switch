#pragma once

#include <stdint.h>

void extension_node_init();

void extension_node_run();

bool extension_node_enqueue_set_report(
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    uint8_t report_type,
    uint8_t const *report_data,
    uint16_t report_data_len
);

bool extension_node_enqueue_set_hid_protocol(
    uint8_t kvm_hid_idx,
    uint8_t hid_protocol
);
