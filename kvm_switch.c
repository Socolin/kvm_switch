#include "kvm_switch.h"

#include <stdlib.h>

#include "hid_manager.h"
#include "logger.h"
#include "usb_device.h"
#include "usb_host.h"
#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"

typedef struct computer_hid_report {
    uint8_t report_id;
    hid_report_type_t report_type;
    uint16_t report_data_len;
    uint8_t *report_data;
    struct computer_hid_report *next;
} computer_hid_report_t;

typedef struct {
    uint8_t computer_id;
    uint8_t hid_protocol_per_interface[CFG_TUH_HID]; // BOOT / REPORT
    computer_hid_report_t *hid_reports_per_interface[CFG_TUH_HID];
} computer_t;

typedef struct {
    uint8_t active_computer_id;
    computer_t computers[MAX_COMPUTER];
} kvm_switch_t;

static kvm_switch_t kvm_switch;

static queue_t kvm_switch_action_queue = {};

// ╔══════════════════════════════════╗
// ║          KVM Switch Logic        ║
// ╚══════════════════════════════════╝

void kvm_switch_init() {
    memset(&kvm_switch, 0, sizeof(kvm_switch));

    for (int i = 0; i < MAX_COMPUTER; i++) {
        kvm_switch.computers[i].computer_id = i;
    }

    queue_init(&kvm_switch_action_queue, sizeof(kvm_switch_action_t), 32);
}

uint64_t last_device_mounted = 0;
uint8_t device_mounted_count = 0;

static void kvm_switch_process_actions() {
    kvm_switch_action_t kvm_switch_action;
    if (queue_try_remove(&kvm_switch_action_queue, &kvm_switch_action)) {
        switch (kvm_switch_action.opcode) {
            case KVM_SWITCH_OP_DEVICE_MOUNT: {
                last_device_mounted = time_us_64();
                device_mounted_count++;
                break;
            }
            case KVM_SWITCH_OP_DEVICE_UMOUNT: {
                assert(device_mounted_count > 0);
                device_mounted_count--;
                break;
            }
            case KVM_SWITCH_OP_HID_MOUNT: {
                const kvm_switch_action_hid_mount_data_t *data = (kvm_switch_action_hid_mount_data_t *)
                        kvm_switch_action.
                        data;
                if (!hid_mgr_register_hid(
                        data->dev_addr,
                        data->host_hid_idx,
                        data->itf_protocol,
                        data->report_desc,
                        data->desc_len)
                ) {
                    free(data->report_desc);
                }
                break;
            }
            case KVM_SWITCH_OP_HID_UMOUNT: {
                const kvm_switch_action_hid_umount_data_t *data = (kvm_switch_action_hid_umount_data_t *)
                        kvm_switch_action.
                        data;
                hid_mgr_unregister_hid(data->dev_addr, data->host_hid_idx);
                break;
            }
            case KVM_SWITCH_OP_HID_REPORT: {
                const kvm_switch_action_hid_report_data_t *data = (kvm_switch_action_hid_report_data_t *)
                        kvm_switch_action.
                        data;
                // FIXME: make this configurable
                if (data->report_data_len > 2 && data->report_data[2] == 0x48 && data->itf_protocol ==
                    HID_ITF_PROTOCOL_KEYBOARD) {
                    log_critical("Resetting PICO in BOOTSEL");
                    multicore_reset_core1();
                    reset_usb_boot(0, 0);
                }

                const hid_t *hid = hid_mgr_get_by_host_idx(data->dev_addr, data->host_hid_idx);
                if (hid == nullptr) {
                    logf_warning("hid_mgr_get_by_host_idx returned nullptr dev_addr: %u host_hid_idx: %u",
                                 data->dev_addr, data->host_hid_idx);
                    return;
                }
                if (!tud_hid_n_ready(hid->kvm_hid_idx)) {
                    logf_warning("tud_hid_ready(%u) == false, skip report", hid->kvm_hid_idx);
                    return;
                }
                if (!tud_hid_n_report(hid->kvm_hid_idx, data->report_id, data->report_data, data->report_data_len)) {
                    log_error("tud_hid_n_report failed");
                }
                break;
            }
            default:
                // FIXME: error
                break;
        }
    }
}

void kvm_switch_task() {
    kvm_switch_process_actions();
    if (last_device_mounted) {
        const uint64_t now = time_us_64();
        // When a device is mounted, wait 1 second before setting up the pico as a usb device.
        // This allow to avoid multiple re-initializations of the usb device each time.
        // When 2 devices are detected, skip the wait
        if (device_mounted_count == 2 || now - last_device_mounted > 1'000'000) {
            last_device_mounted = 0;
            usb_device_connect_to_computer();
        }
    }
}

void kvm_switch_computer_set_hid_protocol(
    const uint8_t computer_id,
    const uint8_t kvm_hid_idx,
    const uint8_t hid_protocol
) {
    assert(computer_id < MAX_COMPUTER);
    assert(kvm_hid_idx < CFG_TUH_HID);

    computer_t *computer = &kvm_switch.computers[computer_id];
    computer->hid_protocol_per_interface[kvm_hid_idx] = hid_protocol;

    if (kvm_switch.active_computer_id == computer_id) {
        usb_host_enqueue_set_protocol(kvm_hid_idx, hid_protocol);
    }
}

void kvm_switch_computer_set_report(
    const uint8_t computer_id,
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint8_t report_type,
    uint8_t const *report_data,
    const uint16_t report_data_len
) {
    assert(computer_id < MAX_COMPUTER);
    assert(kvm_hid_idx < CFG_TUH_HID);

    computer_t *computer = &kvm_switch.computers[computer_id];
    computer_hid_report_t *saved_report = nullptr;

    computer_hid_report_t *itr = computer->hid_reports_per_interface[kvm_hid_idx];
    while (itr != nullptr) {
        // FIXME: Should we check report_type?
        if (itr->report_id == report_id) {
            saved_report = itr;
            break;
        }
        itr = itr->next;
    }

    if (saved_report == nullptr) {
        saved_report = calloc(sizeof(computer_hid_report_t), 1);
        if (saved_report == nullptr) {
            log_critical("Failed to allocate memory for computer_hid_report_t");
            return;
        }
        saved_report->report_id = report_id;
        saved_report->report_type = report_type;
        saved_report->report_data_len = report_data_len;
        saved_report->report_data = malloc(report_data_len);
        if (saved_report->report_data == nullptr) {
            logf_critical("Failed to allocate memory for saved_report->report_data size: %u", report_data_len);
            free(saved_report);
            return;
        }
        memcpy(saved_report->report_data, report_data, report_data_len);
        saved_report->next = computer->hid_reports_per_interface[kvm_hid_idx];
        computer->hid_reports_per_interface[kvm_hid_idx] = saved_report;
    } else {
        if (saved_report->report_data_len < report_data_len) {
            free(saved_report->report_data);
            saved_report->report_data_len = report_data_len;
            saved_report->report_data = malloc(report_data_len);
            if (saved_report->report_data == nullptr) {
                logf_critical("Failed to allocate memory for saved_report->report_data size: %u", report_data_len);
                free(saved_report);
                return;
            }
            memcpy(saved_report->report_data, report_data, report_data_len);
        } else {
            saved_report->report_data_len = report_data_len;
            memcpy(saved_report->report_data, report_data, report_data_len);
        }
    }

    if (kvm_switch.active_computer_id == computer_id) {
        usb_host_enqueue_set_report(kvm_hid_idx, report_id, report_type, report_data, report_data_len);
    }
}

// ╔══════════════════════════════════╗
// ║         KVM switch Action        ║
// ╚══════════════════════════════════╝

static bool kvm_switch_enqueue_action(
    const kvm_switch_action_opcode_t opcode,
    const void *data,
    const size_t data_len

) {
    logf_debug("opcode: %u, data_len: %u", opcode, data_len);
    kvm_switch_action_t action = {
        .opcode = opcode,
        .data_len = data_len,
    };
    memcpy(action.data, data, data_len);

    return queue_try_add(&kvm_switch_action_queue, &action);
}

bool kvm_switch_enqueue_device_mount(
    const uint8_t dev_addr
) {
    const kvm_switch_action_device_mount_data_t action_data = {
        .dev_addr = dev_addr,
    };

    return kvm_switch_enqueue_action(KVM_SWITCH_OP_DEVICE_MOUNT, &action_data, sizeof(action_data));
}

bool kvm_switch_enqueue_device_umount(
    const uint8_t dev_addr
) {
    const kvm_switch_action_device_umount_data_t action_data = {
        .dev_addr = dev_addr,
    };

    return kvm_switch_enqueue_action(KVM_SWITCH_OP_DEVICE_UMOUNT, &action_data, sizeof(action_data));
}

bool kvm_switch_enqueue_hid_mount(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx,
    const uint8_t interface_protocol,
    const uint16_t pid,
    const uint16_t vid,
    const uint8_t *report_desc,
    const uint16_t desc_len
) {
    uint8_t *data_report_desc = malloc(desc_len);
    if (data_report_desc == NULL) {
        log_critical("Not enough memory to mount HID device");
        return false;
    }

    memcpy(data_report_desc, report_desc, desc_len);
    const kvm_switch_action_hid_mount_data_t action_data = {
        .dev_addr = dev_addr,
        .host_hid_idx = host_hid_idx,
        .itf_protocol = interface_protocol,
        .report_desc = data_report_desc,
        .desc_len = desc_len,
        .pid = pid,
        .vid = vid,
    };

    return kvm_switch_enqueue_action(KVM_SWITCH_OP_HID_MOUNT, &action_data, sizeof(action_data));
}

bool kvm_switch_enqueue_hid_umount(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx
) {
    const kvm_switch_action_hid_umount_data_t action_data = {
        .dev_addr = dev_addr,
        .host_hid_idx = host_hid_idx,
    };
    return kvm_switch_enqueue_action(KVM_SWITCH_OP_HID_UMOUNT, &action_data, sizeof(action_data));
}

bool kvm_switch_enqueue_report(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx,
    const uint8_t itf_protocol,
    const uint8_t hid_protocol,
    uint8_t const *report,
    const uint16_t report_len
) {
    const hid_t *hid = hid_mgr_get_by_host_idx(dev_addr, host_hid_idx);
    if (!hid) {
        logf_error("hid not found for dev_addr %u, host_hid_idx %u", dev_addr, host_hid_idx);
        return false;
    }

    // If the interface protocol is report (there is not report_id in boot mode) and the report_descriptor included
    // a report_id, then the report_id is the first byte of the report and need to be extracted.
    const bool use_report_id = hid_protocol == HID_PROTOCOL_REPORT && hid->use_report_id;

    kvm_switch_action_hid_report_data_t action_data = {
        .dev_addr = dev_addr,
        .host_hid_idx = host_hid_idx,
        .report_data_len = use_report_id ? report_len - 1 : report_len,
        .report_id = use_report_id ? report[0] : 0,
        .itf_protocol = itf_protocol,
        .hid_protocol = hid_protocol,
    };
    if (report_len > sizeof(action_data.report_data)) {
        logf_error("report data too long: %u", report_len);
        return false;
    }

    memcpy(
        &action_data.report_data,
        use_report_id ? report + 1 : report,
        action_data.report_data_len
    );

    return kvm_switch_enqueue_action(KVM_SWITCH_OP_HID_REPORT, &action_data, sizeof(action_data));
}
