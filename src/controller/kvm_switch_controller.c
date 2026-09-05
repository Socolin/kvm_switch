#include "kvm_switch_controller.h"

#include <stdlib.h>

#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "class/hid/hid.h"

#include "../shared/hid_manager.h"
#include "../shared/logger.h"
#include "../shared_usb/usb_device.h"

#include "computer_manager.h"
#include "hid_keyboard_report_util.h"
#include "kvm_switch_config.h"
#include "node_link_ctrl.h"
#include "usb_host.h"

typedef struct {
    uint8_t active_computer_id;
    uint64_t last_device_mounted;
    uint64_t usb_device_ready;
    uint8_t device_mounted_count;
    queue_t action_queue;
} kvm_switch_t;

static kvm_switch_t kvm_switch = {};

// ╔══════════════════════════════════╗
// ║          KVM Switch Logic        ║
// ╚══════════════════════════════════╝

void kvm_switch_controller_init() {
    memset(&kvm_switch, 0, sizeof(kvm_switch));
    queue_init(&kvm_switch.action_queue, sizeof(kvm_switch_action_t), 32);
}

static void kvm_switch_ctrl_set_active_computer(
    const uint8_t computer_id
) {
    logf_info("KVM Switch: Setting active computer to %u", computer_id);
    kvm_switch.active_computer_id = computer_id;

    const computer_t *computer = computer_manager_get_computer(computer_id);
    for (uint8_t kvm_hid_idx = 0; kvm_hid_idx < CFG_TUH_HID; kvm_hid_idx++) {
        const hid_t *hid = hid_mgr_get_by_kvm_idx(kvm_hid_idx);
        if (hid == nullptr)
            continue;

        usb_host_enqueue_set_protocol(
            hid->dev_addr,
            hid->host_hid_idx,
            computer->hid_protocol_per_interface[kvm_hid_idx]
        );

        const computer_hid_report_t *report = computer->hid_reports_per_interface[kvm_hid_idx];
        while (report != nullptr) {
            usb_host_enqueue_set_report(
                hid->dev_addr,
                hid->host_hid_idx,
                report->report_id,
                report->report_type,
                report->report_data,
                report->report_data_len
            );
            report = report->next;
        }
    }
}

static void kvm_switch_process_actions() {
    kvm_switch_action_t kvm_switch_action;
    if (queue_try_remove(&kvm_switch.action_queue, &kvm_switch_action)) {
        switch (kvm_switch_action.opcode) {
            case KVM_SWITCH_CONTROLLER_OP_HID_DEVICE_MOUNTED: {
                kvm_switch.last_device_mounted = time_us_64();
                kvm_switch.device_mounted_count++;
                break;
            }
            case KVM_SWITCH_CONTROLLER_OP_HID_DEVICE_UNMOUNTED: {
                assert(kvm_switch.device_mounted_count > 0);
                kvm_switch.device_mounted_count--;
                break;
            }
            case KVM_SWITCH_CONTROLLER_OP_HID_MOUNT: {
                auto const data = (ksc_action_hid_mount_data_t *) kvm_switch_action.data;
                if (!hid_mgr_register_hid(
                        data->dev_addr,
                        data->host_hid_idx,
                        data->itf_protocol,
                        data->vid,
                        data->pid,
                        data->report_desc,
                        data->desc_len)
                ) {
                    free(data->report_desc);
                } else {
                    const hid_t *hid = hid_mgr_get_by_host_idx(data->dev_addr, data->host_hid_idx);
                    node_link_ctrl_enqueue_broadcast_hid_mount(hid);
                }
                break;
            }
            case KVM_SWITCH_CONTROLLER_OP_HID_UMOUNT: {
                auto const data = (ksc_action_hid_umount_data_t *) kvm_switch_action.data;
                hid_mgr_unregister_hid(data->dev_addr, data->host_hid_idx);
                break;
            }
            case KVM_SWITCH_CONTROLLER_OP_HID_REPORT: {
                auto const data = (ksc_action_hid_report_data_t *) kvm_switch_action.data;

                const hid_t *hid = hid_mgr_get_by_host_idx(data->dev_addr, data->host_hid_idx);
                if (hid == nullptr) {
                    logf_warning("hid_mgr_get_by_host_idx returned nullptr dev_addr: %u host_hid_idx: %u",
                                 data->dev_addr, data->host_hid_idx);
                    return;
                }

                uint8_t pressed_keys[16];
                size_t pressed_keys_count = 0;

                if (data->itf_protocol == HID_ITF_PROTOCOL_KEYBOARD && data->hid_protocol == HID_PROTOCOL_BOOT) {
                    pressed_keys_count = hid_report_keyboard_boot_get_pressed_keys(
                        data->report_data,
                        data->report_data_len,
                        pressed_keys,
                        sizeof(pressed_keys)
                    );
                } else if (hid->has_keyboard_report) {
                    pressed_keys_count = hid_report_get_pressed_keys(
                        hid->report_descriptor,
                        data->report_data,
                        data->report_data_len,
                        data->report_id,
                        pressed_keys,
                        sizeof(pressed_keys)
                    );
                }

                if (pressed_keys_count > 0) {
                    const keyboard_shortcut_t *shortcut = kvm_config_first_matching_shortcut(
                        pressed_keys_count,
                        pressed_keys
                    );
                    if (shortcut) {
                        logf_info("Executing shortcut: %d", shortcut->shortcut_id);
                        switch (shortcut->action) {
                            case CHANGE_ACTIVE_COMPUTER_SET: {
                                kvm_switch_ctrl_set_active_computer(shortcut->data[0]);
                                break;
                            }
                            case CHANGE_ACTIVE_COMPUTER_NEXT: {
                                kvm_switch_ctrl_set_active_computer((kvm_switch.active_computer_id + 1) % MAX_COMPUTER);
                                break;
                            }
                            case CHANGE_ACTIVE_COMPUTER_PREVIOUS: {
                                int32_t target_computer_id = kvm_switch.active_computer_id - 1;
                                if (target_computer_id < 0)
                                    target_computer_id = MAX_COMPUTER - 1;
                                kvm_switch_ctrl_set_active_computer(target_computer_id);
                                break;
                            }
                        }
                        break;
                    }
                }
                if (kvm_switch.active_computer_id == LOCAL_COMPUTER_ID) {
                    usb_device_send_report(hid->kvm_hid_idx, data->report_id, data->report_data, data->report_data_len);
                } else {
                    node_link_ctrl_enqueue_send_report(
                        kvm_switch.active_computer_id,
                        hid->kvm_hid_idx,
                        data->report_id,
                        data->report_data,
                        data->report_data_len
                    );
                }
                break;
            }
            case KVM_SWITCH_CONTROLLER_OP_COMPUTER_READY: {
                auto const data = (ksc_action_computer_rdy_data_t *) kvm_switch_action.data;

                logf_info("Computer ready: %u", data->computer_id);

                for (uint8_t kvm_hid_idx = 0; kvm_hid_idx < hid_mgr_get_max_hid_count(); kvm_hid_idx++) {
                    const hid_t *hid = hid_mgr_get_by_kvm_idx(kvm_hid_idx);
                    if (hid == nullptr) {
                        continue;
                    }
                    node_link_ctrl_enqueue_send_hid_mount(data->computer_id, hid);
                }
                if (kvm_switch.usb_device_ready) {
                    const general_config_t *config = kvm_config_get_general();
                    node_link_ctrl_enqueue_send_start_usb_device(data->computer_id, config->vid, config->pid);
                }
                break;
            }
            case KVM_SWITCH_CONTROLLER_OP_USB_DEVICE_MOUNTED: {
                computer_manager_init_computer(LOCAL_COMPUTER_ID);
                break;
            }
            case KVM_SWITCH_CONTROLLER_OP_USB_DEVICE_UNMOUNTED: {
                break;
            }
            default:
                logf_error("Unknown kvm switch action opcode: %u", kvm_switch_action.opcode);
                break;
        }
    }
}

void kvm_switch_controller_task() {
    kvm_switch_process_actions();
    if (kvm_switch.last_device_mounted) {
        const uint64_t now = time_us_64();
        // When a device is mounted, wait 1 second before setting up the pico as a usb device.
        // This allow to avoid multiple re-initializations of the usb device each time.
        // When 2 devices are detected, skip the wait
        if (kvm_switch.device_mounted_count == 2 || now - kvm_switch.last_device_mounted > 1'000'000) {
            kvm_switch.last_device_mounted = 0;
            kvm_switch.usb_device_ready = true;
            const general_config_t *config = kvm_config_get_general();
            usb_device_connect_to_computer(BOARD_TUD_RHPORT, config->vid, config->pid);
            node_link_ctrl_enqueue_broadcast_start_usb_device(config->vid, config->pid);
        }
    }
}

void kvm_switch_controller_computer_set_hid_protocol(
    const uint8_t computer_id,
    const uint8_t kvm_hid_idx,
    const uint8_t hid_protocol
) {
    assert(computer_id < MAX_COMPUTER);
    assert(kvm_hid_idx < CFG_TUH_HID);

    computer_manager_set_hid_protocol(computer_id, kvm_hid_idx, hid_protocol);

    if (kvm_switch.active_computer_id == computer_id) {
        const hid_t *hid = hid_mgr_get_by_kvm_idx(kvm_hid_idx);
        if (hid) {
            usb_host_enqueue_set_protocol(hid->dev_addr, hid->host_hid_idx, hid_protocol);
        }
    }
}

void kvm_switch_controller_computer_set_report(
    const uint8_t computer_id,
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint8_t report_type,
    uint8_t const *report_data,
    const uint16_t report_data_len
) {
    assert(computer_id < MAX_COMPUTER);
    assert(kvm_hid_idx < CFG_TUH_HID);

    if (!computer_manager_set_report(computer_id, kvm_hid_idx, report_id, report_type, report_data, report_data_len)) {
        return;
    }

    if (kvm_switch.active_computer_id == computer_id) {
        const hid_t *hid = hid_mgr_get_by_kvm_idx(kvm_hid_idx);
        if (hid) {
            usb_host_enqueue_set_report(
                hid->dev_addr,
                hid->host_hid_idx,
                report_id,
                report_type,
                report_data,
                report_data_len
            );
        }
    }
}

// ╔══════════════════════════════════╗
// ║         KVM switch Action        ║
// ╚══════════════════════════════════╝

static bool kvm_switch_node_enqueue_action(
    const kvm_switch_node_action_opcode_t opcode,
    const void *data,
    const size_t data_len

) {
    logf_debug("opcode: %u, data_len: %u", opcode, data_len);
    kvm_switch_action_t action = {
        .opcode = opcode,
        .data_len = data_len,
    };

    if (data_len > sizeof(action.data)) {
        logf_error("data_len (%u) exceeds action.data size (%u)", data_len, sizeof(action.data));
        return false;
    }

    memcpy(action.data, data, data_len);

    return queue_try_add(&kvm_switch.action_queue, &action);
}

bool kvm_switch_controller_enqueue_hid_device_mounted(
    const uint8_t dev_addr
) {
    const ksc_action_device_mount_data_t action_data = {
        .dev_addr = dev_addr,
    };

    return kvm_switch_node_enqueue_action(
        KVM_SWITCH_CONTROLLER_OP_HID_DEVICE_MOUNTED,
        &action_data,
        sizeof(action_data)
    );
}

bool kvm_switch_controller_enqueue_hid_device_unmounted(
    const uint8_t dev_addr
) {
    const ksc_action_device_umount_data_t action_data = {
        .dev_addr = dev_addr,
    };

    return kvm_switch_node_enqueue_action(
        KVM_SWITCH_CONTROLLER_OP_HID_DEVICE_UNMOUNTED,
        &action_data,
        sizeof(action_data)
    );
}

bool kvm_switch_controller_enqueue_hid_mount(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx,
    const uint8_t itf_protocol,
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
    const ksc_action_hid_mount_data_t action_data = {
        .dev_addr = dev_addr,
        .host_hid_idx = host_hid_idx,
        .itf_protocol = itf_protocol,
        .report_desc = data_report_desc,
        .desc_len = desc_len,
        .pid = pid,
        .vid = vid,
    };

    return kvm_switch_node_enqueue_action(KVM_SWITCH_CONTROLLER_OP_HID_MOUNT, &action_data, sizeof(action_data));
}

bool kvm_switch_controller_enqueue_hid_umount(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx
) {
    const ksc_action_hid_umount_data_t action_data = {
        .dev_addr = dev_addr,
        .host_hid_idx = host_hid_idx,
    };
    return kvm_switch_node_enqueue_action(KVM_SWITCH_CONTROLLER_OP_HID_UMOUNT, &action_data, sizeof(action_data));
}

bool kvm_switch_controller_enqueue_report(
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

    ksc_action_hid_report_data_t action_data = {
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

    return kvm_switch_node_enqueue_action(KVM_SWITCH_CONTROLLER_OP_HID_REPORT, &action_data, sizeof(action_data));
}

bool kvm_switch_controller_enqueue_computer_ready(
    const uint8_t computer_id
) {
    const ksc_action_computer_rdy_data_t action_data = {
        .computer_id = computer_id,
    };
    return kvm_switch_node_enqueue_action(KVM_SWITCH_CONTROLLER_OP_COMPUTER_READY, &action_data, sizeof(action_data));
}

bool kvm_switch_controller_enqueue_usb_device_mounted() {
    const ksc_usb_device_mounted action_data = {
    };
    return kvm_switch_node_enqueue_action(KVM_SWITCH_CONTROLLER_OP_USB_DEVICE_MOUNTED, &action_data,
                                          sizeof(action_data));
}

bool kvm_switch_controller_enqueue_usb_device_unmounted() {
    const ksc_usb_device_unmounted action_data = {
    };
    return kvm_switch_node_enqueue_action(KVM_SWITCH_CONTROLLER_OP_USB_DEVICE_UNMOUNTED, &action_data,
                                          sizeof(action_data));
}
