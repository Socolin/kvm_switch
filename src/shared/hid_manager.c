#include "hid_manager.h"

#define MAX_HID_COUNT 8

#include <stdlib.h>
#include <string.h>

#include "logger.h"

typedef struct {
    hid_t hid[MAX_HID_COUNT];
} hid_mgr_t;

static hid_mgr_t hid_mgr = {};

void hid_mgr_init() {
    log_info("Initializing HID Manager");

    memset(&hid_mgr, 0, sizeof(hid_mgr));
    for (uint8_t kvm_hid_idx = 0; kvm_hid_idx < MAX_HID_COUNT; kvm_hid_idx++) {
        hid_mgr.hid[kvm_hid_idx].kvm_hid_idx = kvm_hid_idx;
    }
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

        if (type == 1 /*RI_TYPE_GLOBAL*/ && tag == 8 /*RI_GLOBAL_REPORT_ID*/) {
            return true;
        }
    }

    return false;
}

bool hid_mgr_register_hid(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx,
    const uint8_t itf_protocol,
    uint8_t *report_desc,
    const uint16_t report_desc_len
) {
    hid_t *hid = nullptr;
    for (uint8_t kvm_hid_idx = 0; kvm_hid_idx < MAX_HID_COUNT; kvm_hid_idx++) {
        if (hid_mgr.hid[kvm_hid_idx].enabled)
            continue;
        hid = &hid_mgr.hid[kvm_hid_idx];
    }

    if (hid == nullptr) {
        log_critical("Too many HID devices registered");
        return false;
    }

    hid->enabled = true;
    hid->dev_addr = dev_addr;
    hid->host_hid_idx = host_hid_idx;
    hid->itf_protocol = itf_protocol;
    hid->report_desc = report_desc;
    hid->report_desc_len = report_desc_len;
    hid->use_report_id = is_report_id_present_in_descriptor(report_desc, report_desc_len);

    logf_debug("dev_addr: %u, interface_idx: %u, interface_protocol: %u, report_desc_len: %u, use_report_id: %u",
               dev_addr, host_hid_idx, itf_protocol, report_desc_len, hid->use_report_id);

    return true;
}


void hid_mgr_unregister_hid(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx
) {
    for (uint8_t kvm_hid_idx = 0; kvm_hid_idx < MAX_HID_COUNT; kvm_hid_idx++) {
        hid_t *hid = &hid_mgr.hid[kvm_hid_idx];
        if (!hid->enabled)
            continue;

        if (hid->dev_addr == dev_addr && hid->host_hid_idx == host_hid_idx) {
            free(hid->report_desc);
            memset(hid, 0, sizeof(hid_t));
            hid->kvm_hid_idx = kvm_hid_idx;
            return;
        }
    }
}

const hid_t *hid_mgr_get_by_host_idx(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx
) {
    for (uint8_t kvm_hid_idx = 0; kvm_hid_idx < MAX_HID_COUNT; kvm_hid_idx++) {
        const hid_t *hid = &hid_mgr.hid[kvm_hid_idx];
        if (!hid->enabled)
            continue;

        if (hid->dev_addr == dev_addr && hid->host_hid_idx == host_hid_idx) {
            return hid;
        }
    }

    return nullptr;
}

const hid_t *hid_mgr_get_by_kvm_idx(
    const uint8_t kvm_hid_idx
) {
    if (kvm_hid_idx > MAX_HID_COUNT)
        return nullptr;

    const hid_t *hid = &hid_mgr.hid[kvm_hid_idx];
    if (!hid->enabled)
        return nullptr;

    return hid;
}

uint8_t hid_mgr_get_max_hid_count() {
    return MAX_HID_COUNT;
}
