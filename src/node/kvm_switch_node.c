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
        case KVM_SWITCH_CONTROLLER_OP_HID_MOUNT:
            // hid_mgr_register_hid();
            break;
        case KVM_SWITCH_CONTROLLER_OP_HID_UMOUNT:
            // hid_mgr_unregister_hid();
            break;
        case KVM_SWITCH_CONTROLLER_OP_HID_REPORT:
            // usb_device_send_report();
            break;
        case KVM_SWITCH_CONTROLLER_OP_CONNECT_USB_DEVICE:
            usb_device_connect_to_computer(0);
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

static bool kvm_switch_enqueue_action(
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
