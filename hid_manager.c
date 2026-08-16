#include "hid_manager.h"

#define MAX_HID_COUNT CFG_TUH_HID

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "logger.h"
#include "class/hid/hid.h"


typedef struct {
    hid_t hid[MAX_HID_COUNT];
} hid_mgr_t;

static hid_mgr_t hid_mgr = {0};

void hid_mgr_init() {
    log_info("Initializing HID Manager");

    memset(&hid_mgr, 0, sizeof(hid_mgr));
}

// See hid1_11.pdf
static bool is_report_id_present_in_descriptor(
    const uint8_t *report_desc,
    const uint16_t desc_len
) {
    uint16_t i = 0;

    while (i < desc_len) {
        const uint8_t prefix = report_desc[i++];

        // Skip unused "Long items"
        if (prefix == 0xFE) {
            if (i + 2 > desc_len) {
                return false;
            }

            const uint8_t data_size = report_desc[i++];
            [[maybe_unused]] const uint8_t long_tag = report_desc[i++];

            if (i + data_size > desc_len) {
                return false;
            }

            i += data_size;
            continue;
        }

        const uint8_t size_code = prefix & 0x3;
        const uint8_t type = (prefix >> 2) & 0x3;
        const uint8_t tag = (prefix >> 4) & 0xf;

        uint8_t data_size = 0;
        switch (size_code) {
            case 0: data_size = 0;
                break;
            case 1: data_size = 1;
                break;
            case 2: data_size = 2;
                break;
            case 3: data_size = 4;
                break;
            default:
                return false;
        }

        if (i + data_size > desc_len)
            return false;

        if (type == RI_TYPE_GLOBAL && tag == RI_GLOBAL_REPORT_ID) {
            return true;
        }
    }

    return false;
}

bool hid_mgr_register_hid(
    const uint8_t dev_addr,
    const uint8_t interface_idx,
    const uint8_t interface_protocol,
    uint8_t *report_desc,
    const uint16_t report_desc_len
) {
    hid_t *hid = nullptr;
    for (uint8_t hid_idx = 0; hid_idx < MAX_HID_COUNT; hid_idx++) {
        if (hid_mgr.hid[hid_idx].enabled)
            continue;
        hid = &hid_mgr.hid[hid_idx];
    }

    if (hid == nullptr) {
        log_critical("Too many HID devices registered");
        return false;
    }

    hid->enabled = true;
    hid->dev_addr = dev_addr;
    hid->interface_idx = interface_idx;
    hid->interface_protocol = interface_protocol;
    hid->report_desc = report_desc;
    hid->report_desc_len = report_desc_len;
    hid->use_report_id = is_report_id_present_in_descriptor(report_desc, report_desc_len);

    logf_debug("dev_addr: %u, interface_idx: %u, interface_protocol: %u, report_desc_len: %u, use_report_id: %u", dev_addr, interface_idx, interface_protocol, report_desc_len, hid->use_report_id);

    return true;
}


void hid_mgr_unregister_hid(
    const uint8_t dev_addr,
    const uint8_t interface_idx
) {
    for (uint8_t hid_idx = 0; hid_idx < MAX_HID_COUNT; hid_idx++) {
        hid_t *hid = &hid_mgr.hid[hid_idx];
        if (!hid->enabled)
            continue;

        if (hid->dev_addr == dev_addr && hid->interface_idx == interface_idx) {
            free(hid->report_desc);
            memset(hid, 0, sizeof(hid_t));
            return;
        }
    }
}

bool hid_mgr_is_hid_using_report_id(
    const uint8_t dev_addr,
    const uint8_t interface_idx
) {
    for (uint8_t hid_idx = 0; hid_idx < MAX_HID_COUNT; hid_idx++) {
        const hid_t *hid = &hid_mgr.hid[hid_idx];
        if (!hid->enabled)
            continue;

        if (hid->dev_addr == dev_addr && hid->interface_idx == interface_idx) {
            return hid->use_report_id;
        }
    }
    return false;
}

int hid_mgr_get_hid_idx(
    const uint8_t dev_addr,
    const uint8_t interface_idx
) {
    for (uint8_t hid_idx = 0; hid_idx < MAX_HID_COUNT; hid_idx++) {
        const hid_t *hid = &hid_mgr.hid[hid_idx];
        if (!hid->enabled)
            continue;

        if (hid->dev_addr == dev_addr && hid->interface_idx == interface_idx) {
            return hid_idx;
        }
    }

    return 0;
}

const hid_t* hid_mgr_get(
    const uint8_t hid_idx
) {
    if (hid_idx > MAX_HID_COUNT)
        return nullptr;

    const hid_t *hid = &hid_mgr.hid[hid_idx];
    if (!hid->enabled)
        return nullptr;

    return hid;
}

uint8_t hid_mgr_get_max_hid_count() {
    return MAX_HID_COUNT;
}