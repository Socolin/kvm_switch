#pragma once

#include <stdint.h>

typedef struct {
    uint8_t enabled; /**< true when this entry is used, false if it's not used */
    uint8_t kvm_hid_idx; /**< The index of the HID interface in the KVM (Exposed to the computers) */
    uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    uint8_t host_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
    uint8_t itf_protocol; /**< Interface protocol \see hid_interface_protocol_enum_t */
    uint8_t *report_desc; /**< Pointer to the report descriptor (malloced) */
    uint16_t report_desc_len;
    bool use_report_id; /**< **true** when the HID descriptor include one or multiple report ID */
} hid_t;

void hid_mgr_init();

bool hid_mgr_register_hid(
    uint8_t dev_addr,
    uint8_t host_hid_idx,
    uint8_t itf_protocol,
    uint8_t *report_desc,
    uint16_t report_desc_len
);

void hid_mgr_unregister_hid(
    uint8_t dev_addr,
    uint8_t host_hid_idx
);

const hid_t* hid_mgr_get_by_host_idx(
    uint8_t dev_addr,
    uint8_t host_hid_idx
);

const hid_t* hid_mgr_get_by_kvm_idx(
    uint8_t kvm_hid_idx
);

uint8_t hid_mgr_get_max_hid_count();