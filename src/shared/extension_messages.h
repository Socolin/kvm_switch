#pragma once

#include <stddef.h>
#include <stdint.h>

/*
 * Extension Messages
 *
 * Contains messages exchanged between the extension node and the controller node.
 */

// ╔══════════════════════════════════╗
// ║        Extension Messages        ║
// ╚══════════════════════════════════╝

#define EXTENSION_MESSAGE_HEADER 0x42

typedef enum {
    EXTENSION_PROTOCOL_OPCODE_EMPTY,
    EXTENSION_PROTOCOL_OPCODE_ACK,
    EXTENSION_PROTOCOL_OPCODE_NACK,
} extension_protocol_opcode_t;

typedef enum {
    EXTENSION_HOST_MESSAGE_OP_HID_MOUNT,
    EXTENSION_HOST_MESSAGE_OP_HID_UMOUNT,
    EXTENSION_HOST_MESSAGE_OP_HID_REPORT,
    EXTENSION_HOST_MESSAGE_OP_START_USB_DEVICE,
} extension_host_message_opcode_t;

typedef enum {
    EXTENSION_NODE_MESSAGE_OP_SET_REPORT,
    EXTENSION_NODE_MESSAGE_OP_SET_HID_PROTOCOL,
} extension_node_message_opcode_t;

typedef struct __attribute__((packed)) {
    uint8_t header; // Always 0x42
    uint8_t protocol_opcode; //
    size_t message_len;
    uint16_t message_crc;
    // Keep header_crc at the end of the struct
    uint16_t header_crc;
} extension_message_transport_header_t;

// This allow to send larger chunk of data that are malloced
typedef struct __attribute__((packed)) {
    uint16_t data_len;
    uint8_t *data;
    bool should_free_once_sent;
} extension_message_extra_data_t;

typedef struct __attribute__((packed)) {
    uint8_t opcode; /**< \see extension_host_message_opcode_t and \see extension_node_message_opcode_t */
    uint16_t data_len;
    uint8_t extra_data_count;
} extension_message_header_t;

typedef struct __attribute__((packed)) {
    extension_message_transport_header_t transport_header;
    extension_message_header_t header;
    extension_message_extra_data_t extra_data[1]; // For now only 1 extra data is possible
    uint8_t data[128];
} extension_message_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
    const uint8_t itf_protocol; /**< Interface protocol \see hid_interface_protocol_enum_t */
    const uint16_t vid; /**< The USB vendor id */
    const uint16_t pid; /**< The USB product id */
    const uint16_t desc_len; /**< HID report descriptor length */
    uint8_t *report_desc; /**< HID report descriptor. (malloced, need to be freed by consumer). \see hid1_11.pdf */
} extension_message_hid_mount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
} extension_message_device_mount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
} extension_message_hid_umount_data_t;

typedef struct {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
} extension_message_device_umount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx;; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
    const uint8_t itf_protocol; /**< Interface protocol \see hid_interface_protocol_enum_t */
    const uint8_t hid_protocol; /**< HID protocol (boot / report) \see hid_protocol_mode_enum_t */
    const uint8_t report_id; /**< The report ID if any. 0 = no report id */
    const uint16_t report_data_len; /**< The length of the report data */
    uint8_t report_data[96]; /**< The report data */
} extension_message_hid_report_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t kvm_hid_idx; /**< The index of the HID interface in the KVM (Exposed to the computers) */
    const uint8_t report_id; /**< The report ID if any. 0 = no report id */
    const uint8_t report_type; /**< The report type \see hid_report_type_t */
    /* The report data is sent as extra data, size is variable to this one is malloced*/
    // const uint16_t report_data_len; /**< The length of the report data */
    // uint8_t *report_data; /**< The report data */
} extension_message_hid_set_report_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t kvm_hid_idx; /**< The index of the HID interface in the KVM (Exposed to the computers) */
    const uint8_t hid_protocol; /**< HID protocol (boot / report) \see hid_protocol_mode_enum_t */
} extension_message_hid_set_hid_protocol_data_t;
