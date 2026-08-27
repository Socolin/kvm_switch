#pragma once

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

void kvm_switch_node_usb_device_mounted();

void kvm_switch_node_usb_device_unmounted();

// ╔══════════════════════════════════╗
// ║         KVM switch Action        ║
// ╚══════════════════════════════════╝

typedef enum {
    KVM_SWITCH_NODE_OP_HID_MOUNT,
    KVM_SWITCH_NODE_OP_HID_UMOUNT,
    KVM_SWITCH_NODE_OP_HID_REPORT,
    KVM_SWITCH_NODE_OP_CONNECT_USB_DEVICE,
} kvm_switch_node_action_opcode_t;

typedef struct {
    kvm_switch_node_action_opcode_t opcode;
    uint8_t data[128];
    uint32_t data_len;
} kvm_switch_action_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
    const uint8_t kvm_hid_idx; /**< The index of the HID interface in the KVM (Exposed to the computers) */
    const uint8_t itf_protocol; /**< Interface protocol \see hid_interface_protocol_enum_t */
    const uint16_t vid; /**< The USB vendor id */
    const uint16_t pid; /**< The USB product id */
    uint8_t *const report_desc;
    /**< HID report descriptor. (malloced, need to be freed by consumer). \see hid1_11.pdf */
    const uint16_t desc_len; /**< HID report descriptor length */
} kvm_switch_node_action_hid_mount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
} kvm_switch_node_action_hid_umount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t kvm_hid_idx;
    const uint8_t report_id;
    const uint16_t report_data_len;
    uint8_t report_data[96];
} kvm_switch_node_action_hid_report_data_t;

typedef struct __attribute__((packed)) {
    const uint16_t vid;
    const uint16_t pid;
} kvm_switch_node_action_connect_usb_device_data_t;

void kvm_switch_node_enqueue_hid_mount(
    uint8_t dev_addr,
    uint8_t host_hid_idx,
    uint8_t kvm_hid_idx,
    uint8_t itf_protocol,
    uint16_t vid,
    uint16_t pid,
    uint8_t *report_desc,
    uint16_t report_desc_len
);

void kvm_switch_node_enqueue_hid_umount(
    uint8_t dev_addr,
    uint8_t host_hid_idx
);

void kvm_switch_node_enqueue_hid_report(
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    uint16_t report_data_len,
    const uint8_t *report_data
);

void kvm_switch_node_enqueue_connect_usb_device(
    uint16_t vid,
    uint16_t pid
);
