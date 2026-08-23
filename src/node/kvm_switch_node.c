#include "kvm_switch_node.h"

#include <string.h>

#include "extension_node.h"
#include "hid_manager.h"
#include "logger.h"
#include "../shared_usb/usb_device.h"
#include "pico/util/queue.h"

typedef struct {
    queue_t action_queue;
} kvm_switch_node_t;

static kvm_switch_node_t kvm_switch;

void kvm_switch_node_init() {
    queue_init(&kvm_switch.action_queue, sizeof(kvm_switch_action_t), 16);
}

void kvm_switch_node_task() {
    kvm_switch_action_t action;
    if (!queue_try_remove(&kvm_switch.action_queue, &action))
        return;

    switch (action.opcode) {
        case KVM_SWITCH_CONTROLLER_OP_HID_MOUNT: {
            const kvm_switch_node_action_hid_mount_data_t *action_data = (kvm_switch_node_action_hid_mount_data_t *)
                    action.data;
            hid_mgr_register_hid(
                action_data->dev_addr, // Not need on node side
                action_data->host_hid_idx,
                action_data->kvm_hid_idx,
                action_data->itf_protocol,
                action_data->report_desc,
                action_data->report_desc_len
            );
            break;
        }
        case KVM_SWITCH_CONTROLLER_OP_HID_UMOUNT: {
            const kvm_switch_node_action_hid_umount_data_t *action_data = (kvm_switch_node_action_hid_umount_data_t *)
                    action.data;
            hid_mgr_unregister_hid(action_data->dev_addr, action_data->host_hid_idx);
            break;
        }
        case KVM_SWITCH_CONTROLLER_OP_HID_REPORT: {
            const kvm_switch_node_action_hid_report_data_t *action_data = (kvm_switch_node_action_hid_report_data_t *)
                    action.data;
            usb_device_send_report(
                action_data->kvm_hid_idx,
                action_data->report_id,
                action_data->report_data,
                action_data->report_data_len
            );
            break;
        }
        case KVM_SWITCH_CONTROLLER_OP_CONNECT_USB_DEVICE:
            const kvm_switch_node_action_connect_usb_device_data_t *action_data = (
                kvm_switch_node_action_connect_usb_device_data_t *) action.data;
            usb_device_connect_to_computer(0, action_data->vid, action_data->pid);
            break;
    }
}

void kvm_switch_node_computer_set_hid_protocol(
    const uint8_t kvm_hid_idx,
    const uint8_t hid_protocol
) {
    if (!extension_node_enqueue_set_hid_protocol(kvm_hid_idx, hid_protocol)) {
        logf_error("Failed to enqueue HID protocol");
    }
}

void kvm_switch_node_computer_set_report(
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint8_t report_type,
    uint8_t const *report_data,
    const uint16_t report_data_len
) {
    if (!extension_node_enqueue_set_report(kvm_hid_idx, report_id, report_type, report_data, report_data_len)) {
        logf_error("Failed to enqueue report");
    }
}

// ╔══════════════════════════════════╗
// ║         KVM switch Action        ║
// ╚══════════════════════════════════╝

static bool kvm_switch_node_enqueue_action(
    const kvm_switch_controller_action_opcode_t opcode,
    const void *data,
    const size_t data_len

) {
    logf_debug("opcode: %u, data_len: %u", opcode, data_len);
    kvm_switch_action_t action = {
        .opcode = opcode,
        .data_len = data_len,
    };
    memcpy(action.data, data, data_len);

    return queue_try_add(&kvm_switch.action_queue, &action);
}

void kvm_switch_node_enqueue_hid_mount(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx,
    const uint8_t kvm_hid_idx,
    const uint8_t itf_protocol,
    const uint16_t vid,
    const uint16_t pid,
    uint8_t *const report_desc, // FIXME: Replace this with malloc_uint8_t or something like that
    const uint16_t report_desc_len
) {
    const kvm_switch_node_action_hid_mount_data_t action_data = {
        .dev_addr = dev_addr,
        .host_hid_idx = host_hid_idx,
        .kvm_hid_idx = kvm_hid_idx,
        .itf_protocol = itf_protocol,
        .report_desc = report_desc,
        .desc_len = report_desc_len,
        .pid = pid,
        .vid = vid,
    };

    kvm_switch_node_enqueue_action(KVM_SWITCH_CONTROLLER_OP_HID_MOUNT, &action_data, sizeof(action_data));
}

void kvm_switch_node_enqueue_hid_umount(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx
) {
    const kvm_switch_node_action_hid_umount_data_t action_data = {
        .dev_addr = dev_addr,
        .host_hid_idx = host_hid_idx,
    };
    kvm_switch_node_enqueue_action(KVM_SWITCH_CONTROLLER_OP_HID_UMOUNT, &action_data, sizeof(action_data));
}

void kvm_switch_node_enqueue_hid_report(
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint16_t report_data_len,
    const uint8_t *const report_data
) {
    kvm_switch_node_action_hid_report_data_t action_data = {
        .kvm_hid_idx = kvm_hid_idx,
        .report_id = report_id,
        .report_data_len = report_data_len,
    };
    if (report_data_len > sizeof(action_data.report_data)) {
        logf_critical("Report data length exceeds buffer size: %u", report_data_len);
        return;
    }
    memcpy(action_data.report_data, report_data, report_data_len);
    kvm_switch_node_enqueue_action(KVM_SWITCH_CONTROLLER_OP_HID_REPORT, &action_data, sizeof(action_data));
}

void kvm_switch_node_enqueue_connect_usb_device() {
    const kvm_switch_node_action_connect_usb_device_data_t action_data = {
    };
    kvm_switch_node_enqueue_action(KVM_SWITCH_CONTROLLER_OP_CONNECT_USB_DEVICE, &action_data, sizeof(action_data));
}
