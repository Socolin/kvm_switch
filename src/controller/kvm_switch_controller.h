#ifndef HID_H
#define HID_H
#include <stdint.h>

#include "tusb.h"

// ╔══════════════════════════════════╗
// ║          KVM Switch Logic        ║
// ╚══════════════════════════════════╝

void kvm_switch_controller_init();

void kvm_switch_controller_task();

/**
 * @param computer_id The computer id, 0 is the one on the main board
 * @param kvm_hid_idx The index of the HID interface in the KVM (Exposed to the computers)
 * @param hid_protocol The HID protocol (boot / report) \see hid_protocol_mode_enum_t
 */
void kvm_switch_controller_computer_set_hid_protocol(
    uint8_t computer_id,
    uint8_t kvm_hid_idx,
    uint8_t hid_protocol
);

/**
 * @param computer_id The computer id, 0 is the one on the main board
 * @param kvm_hid_idx The index of the HID interface in the KVM (Exposed to the computers)
 * @param report_id The report id of the report
 * @param report_type \see hid_report_type_t
 * @param report_data The data of the report
 * @param report_data_len The length of the report data
 */
void kvm_switch_controller_computer_set_report(
    uint8_t computer_id,
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
    KVM_SWITCH_CONTROLLER_OP_DEVICE_MOUNT,
    KVM_SWITCH_CONTROLLER_OP_DEVICE_UMOUNT,
    KVM_SWITCH_CONTROLLER_OP_HID_MOUNT,
    KVM_SWITCH_CONTROLLER_OP_HID_UMOUNT,
    KVM_SWITCH_CONTROLLER_OP_HID_REPORT,
} kvm_switch_controller_action_opcode_t;

typedef struct {
    kvm_switch_controller_action_opcode_t opcode;
    size_t data_len;
    uint8_t data[128];
} kvm_switch_action_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
    const uint8_t itf_protocol; /**< Interface protocol \see hid_interface_protocol_enum_t */
    const uint16_t vid; /**< The USB vendor id */
    const uint16_t pid; /**< The USB product id */
    uint8_t *report_desc; /**< HID report descriptor. (malloced, need to be freed by consumer). \see hid1_11.pdf */
    const uint16_t desc_len; /**< HID report descriptor length */
} kvm_switch_controller_action_hid_mount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
} kvm_switch_controller_action_device_mount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
} kvm_switch_controller_action_hid_umount_data_t;

typedef struct {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
} kvm_switch_controller_action_device_umount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx;; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
    const uint8_t itf_protocol; /**< Interface protocol \see hid_interface_protocol_enum_t */
    const uint8_t hid_protocol; /**< HID protocol (boot / report) \see hid_protocol_mode_enum_t */
    const uint8_t report_id; /**< The report ID if any. 0 = no report id */
    const uint16_t report_data_len; /**< The length of the report data */
    uint8_t report_data[96]; /**< The report data */
} kvm_switch_controller_action_hid_report_data_t;

/**
 * @param dev_addr Device address (Which port the device is connected. Values: 1, 2)
 * @return
 *   - **true**: if the message was enqueued successfully
 *   - **false**: otherwise
 */
bool kvm_switch_controller_enqueue_device_mount(
    uint8_t dev_addr
);

/**
 * @param dev_addr Device address (Which port the device is connected. Values: 1, 2)
 * @return
 *   - **true**: if the message was enqueued successfully
 *   - **false**: otherwise
 */
bool kvm_switch_controller_enqueue_device_umount(
    uint8_t dev_addr
);

/**
 * @param dev_addr Device address (Which port the device is connected. Values: 1, 2)
 * @param host_hid_idx HID interface index on USB Host side (Where the keyboard / mouse are connected)
 * @param itf_protocol Interface protocol \see hid_interface_protocol_enum_t
 * @param pid USB Product ID
 * @param vid USB Vendor ID
 * @param report_desc Report descriptor \see hid1_11.pdf
 * @param desc_len Report descriptor length
 * @return
 *   - **true**: if the message was enqueued successfully
 *   - **false**: otherwise
 */
bool kvm_switch_controller_enqueue_hid_mount(
    uint8_t dev_addr,
    uint8_t host_hid_idx,
    uint8_t itf_protocol,
    uint16_t pid,
    uint16_t vid,
    const uint8_t *report_desc,
    uint16_t desc_len
);

/**
 * @param dev_addr Device address (Which port the device is connected. Values: 1, 2)
 * @param host_hid_idx HID interface index on USB Host side (Where the keyboard / mouse are connected)
 * @return
 *   - **true**: if the message was enqueued successfully
 *   - **false**: otherwise
 */
bool kvm_switch_controller_enqueue_hid_umount(
    uint8_t dev_addr,
    uint8_t host_hid_idx
);

/**
 * @param dev_addr Device address (Which port the device is connected. Values: 1, 2)
 * @param host_hid_idx HID interface index on USB Host side (Where the keyboard / mouse are connected)
 * @param itf_protocol Interface protocol \see hid_interface_protocol_enum_t
 * @param hid_protocol HID protocol \see hid_protocol_mode_enum_t
 * @param report HID report
 * @param report_len HID report length
 * @return
 *   - **true**: if the message was enqueued successfully
 *   - **false**: otherwise
 */
bool kvm_switch_controller_enqueue_report(
    uint8_t dev_addr,
    uint8_t host_hid_idx,
    uint8_t itf_protocol,
    uint8_t hid_protocol,
    uint8_t const *report,
    uint16_t report_len
);

#endif
