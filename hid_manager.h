#pragma once

#include <stdint.h>

typedef struct {
    uint8_t enabled;
    uint8_t dev_addr;
    uint8_t interface_idx;
    uint8_t interface_protocol;
    uint8_t *report_desc;
    uint16_t report_desc_len;
    bool use_report_id;
} hid_t;

void hid_mgr_init();

bool hid_mgr_register_hid(
    uint8_t dev_addr,
    uint8_t interface_idx,
    uint8_t interface_protocol,
    uint8_t *report_desc,
    uint16_t report_desc_len
);

void hid_mgr_unregister_hid(
    uint8_t dev_addr,
    uint8_t interface_idx
);

bool hid_mgr_is_hid_using_report_id(
    uint8_t dev_addr,
    uint8_t interface_idx
);

int hid_mgr_get_hid_idx(
    uint8_t dev_addr,
    uint8_t interface_idx
);

const hid_t* hid_mgr_get(
    uint8_t hid_idx
);

uint8_t hid_mgr_get_max_hid_count();