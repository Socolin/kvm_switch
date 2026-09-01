#include "hid_manager.h"

#define MAX_HID_COUNT 8

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hid_keyboard_report_util.h"
#include "logger.h"
#include "hid_report_descriptor.h"

typedef struct {
    hid_t hid[MAX_HID_COUNT];
} hid_mgr_t;

static hid_mgr_t hid_mgr = {};

void hid_mgr_init() {
    log_info("Initializing HID Manager");

    memset(&hid_mgr, 0, sizeof(hid_mgr));
    for (uint8_t kvm_hid_idx = 0; kvm_hid_idx < MAX_HID_COUNT; kvm_hid_idx++) {
        hid_mgr.hid[kvm_hid_idx].kvm_hid_idx = kvm_hid_idx;
        hid_mgr.hid[kvm_hid_idx].enabled = false;
    }
}


bool hid_mgr_register_at_hid(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx,
    const uint8_t kvm_hid_idx,
    const uint8_t itf_protocol,
    const uint16_t vid,
    const uint16_t pid,
    uint8_t *report_desc,
    const uint16_t report_desc_len
) {
    assert(kvm_hid_idx < MAX_HID_COUNT);

    hid_t *hid = &hid_mgr.hid[kvm_hid_idx];

    if (hid->enabled) {
        free(hid->raw_report_descriptor);
        hid_report_descriptor_free(hid->report_descriptor);
    }

    hid->enabled = true;
    hid->dev_addr = dev_addr;
    hid->host_hid_idx = host_hid_idx;
    hid->itf_protocol = itf_protocol;
    hid->raw_report_descriptor = report_desc;
    hid->vid = vid;
    hid->pid = pid;
    hid->raw_report_descriptor_len = report_desc_len;
    hid->report_descriptor = hid_report_descriptor_parse(report_desc, report_desc_len);
    if (hid->report_descriptor) {
        hid->has_keyboard_report = hid_report_descriptor_contains_keycodes(hid->report_descriptor);
    }
    hid->use_report_id = hid_report_descriptor_is_report_id_present(report_desc, report_desc_len);

    logf_debug("dev_addr: %u, interface_idx: %u, itf_protocol: %u, report_desc_len: %u, use_report_id: %u",
               dev_addr, host_hid_idx, itf_protocol, report_desc_len, hid->use_report_id);

    return true;
}

bool hid_mgr_register_hid(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx,
    const uint8_t itf_protocol,
    const uint16_t vid,
    const uint16_t pid,
    uint8_t *report_desc,
    const uint16_t report_desc_len
) {
    logf_debug("dev_addr: %u, host_hid_idx: %u", dev_addr, host_hid_idx);
    const hid_t *hid = nullptr;
    uint8_t kvm_hid_idx = 0;
    for (; kvm_hid_idx < MAX_HID_COUNT; kvm_hid_idx++) {
        if (hid_mgr.hid[kvm_hid_idx].enabled)
            continue;
        hid = &hid_mgr.hid[kvm_hid_idx];
        break;
    }

    if (hid == nullptr) {
        log_critical("Too many HID devices registered");
        return false;
    }

    return hid_mgr_register_at_hid(
        dev_addr,
        host_hid_idx,
        kvm_hid_idx,
        itf_protocol,
        vid,
        pid,
        report_desc,
        report_desc_len
    );
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
            free(hid->raw_report_descriptor);
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
    if (kvm_hid_idx >= MAX_HID_COUNT)
        return nullptr;

    const hid_t *hid = &hid_mgr.hid[kvm_hid_idx];
    if (!hid->enabled)
        return nullptr;

    return hid;
}

uint8_t hid_mgr_get_max_hid_count() {
    return MAX_HID_COUNT;
}
