#ifndef HID_H
#define HID_H
#include <stdint.h>

#include "tusb.h"

#define MAX_COMPUTER 2

// ╔══════════════════════════════════╗
// ║          KVM Switch Logic        ║
// ╚══════════════════════════════════╝

void kvm_switch_init();

void kvm_switch_task();

void kvm_switch_computer_set_hid_protocol(
    uint8_t computer_id,
    uint8_t kvm_hid_idx,
    uint8_t protocol
);

void kvm_switch_computer_set_report(
    uint8_t computer_id,
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    uint8_t report_type, // hid_report_type_t
    uint8_t const *report_data,
    uint16_t report_data_len
);

// ╔══════════════════════════════════╗
// ║         KVM switch Action        ║
// ╚══════════════════════════════════╝

typedef enum {
    KVM_SWITCH_DEVICE_MOUNT,
    KVM_SWITCH_DEVICE_UMOUNT,
    KVM_SWITCH_HID_MOUNT,
    KVM_SWITCH_HID_UMOUNT,
    KVM_SWITCH_HID_REPORT,
} kvm_switch_action_opcode_t;

typedef struct {
    kvm_switch_action_opcode_t opcode;
    size_t data_len;
    uint8_t data[128];
} kvm_switch_action_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr;
    const uint8_t itf_idx;
    const uint8_t itf_protocol;
    const uint16_t pid;
    const uint16_t vid;
    // Need to be malloc() / free();
    uint8_t *report_desc;
    const uint16_t desc_len;
} kvm_switch_action_hid_mount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr;
} kvm_switch_action_device_mount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr;
    const uint8_t idx;
} kvm_switch_action_hid_umount_data_t;

typedef struct {
    const uint8_t dev_addr;
} kvm_switch_action_device_umount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr;
    const uint8_t idx;
    const uint8_t itf_protocol;
    const uint8_t hid_protocol;
    const uint8_t report_id;
    const uint16_t report_data_len;
    uint8_t report_data[96];
} kvm_switch_action_hid_report_data_t;


bool kvm_switch_enqueue_device_mount(
    uint8_t dev_addr
);

bool kvm_switch_enqueue_device_umount(
    uint8_t dev_addr
);

bool kvm_switch_enqueue_hid_mount(
    uint8_t dev_addr,
    uint8_t hid_interface_idx, // FIXME: host_hid_idx ?
    uint8_t interface_protocol,
    uint16_t pid,
    uint16_t vid,
    const uint8_t *report_desc,
    uint16_t desc_len
);

bool kvm_switch_enqueue_hid_umount(
    uint8_t dev_addr,
    uint8_t hid_interface_idx
);

bool kvm_switch_enqueue_report(
    uint8_t dev_addr,
    uint8_t hid_interface_idx,
    uint8_t itf_protocol,
    uint8_t hid_protocol,
    uint8_t const *report,
    uint16_t report_len
);


#endif
