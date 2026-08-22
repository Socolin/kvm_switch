#pragma once
#include <stddef.h>
#include <stdint.h>

// ╔══════════════════════════════════╗
// ║          KVM Switch Logic        ║
// ╚══════════════════════════════════╝

void kvm_switch_node_init();

void kvm_switch_node_task();

/**
 * @param kvm_hid_idx The index of the HID interface in the KVM (Exposed to the computers)
 * @param hid_protocol The HID protocol (boot / report) \see hid_protocol_mode_enum_t
 */
void kvm_switch_node_computer_set_hid_protocol(
    uint8_t kvm_hid_idx,
    uint8_t hid_protocol
);

/**
 * @param kvm_hid_idx The index of the HID interface in the KVM (Exposed to the computers)
 * @param report_id The report id of the report
 * @param report_type \see hid_report_type_t
 * @param report_data The data of the report
 * @param report_data_len The length of the report data
 */
void kvm_switch_node_computer_set_report(
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    uint8_t report_type,
    uint8_t const *report_data,
    uint16_t report_data_len
);

// ╔══════════════════════════════════╗
// ║         KVM switch Action        ║
// ╚══════════════════════════════════╝

typedef enum {
    KVM_SWITCH_CONTROLLER_OP_HID_MOUNT,
    KVM_SWITCH_CONTROLLER_OP_HID_UMOUNT,
    KVM_SWITCH_CONTROLLER_OP_HID_REPORT,
    KVM_SWITCH_CONTROLLER_OP_CONNECT_USB_DEVICE,
} kvm_switch_controller_action_opcode_t;

typedef struct {
    kvm_switch_controller_action_opcode_t opcode;
    uint8_t data[128];
    size_t data_len;
} kvm_switch_action_t;

void kvm_switch_node_enqueue_hid_mount();

void kvm_switch_node_enqueue_hid_umount();

void kvm_switch_node_enqueue_hid_report();

void kvm_switch_node_enqueue_connect_usb_device();
