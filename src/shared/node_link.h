#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hardware/spi.h"

/*
 * Node link
 *
 * Contains basic messages exchanged between the extension node and the controller node.
 */

#define NODE_LINK_SPI_BAUD_RATE 4'000'000
#define MAX_WAIT_FOR_SPI_READY_US 100'000


// ╔══════════════════════════════════╗
// ║        Node Link Messages        ║
// ╚══════════════════════════════════╝

// ┌──────────────────────────────────┐
// │               Base               │
// └──────────────────────────────────┘

#define NL_MESSAGE_HEADER 0x42

typedef enum {
    NL_TRANSPORT_OPCODE_DATA,
    NL_TRANSPORT_OPCODE_ACK,
    NL_TRANSPORT_OPCODE_NACK,
} node_link_transport_opcode_t;

typedef enum {
    NL_CTRL_MESSAGE_OP_INIT,
    NL_CTRL_MESSAGE_OP_HID_MOUNT,
    NL_CTRL_MESSAGE_OP_HID_UMOUNT,
    NL_CTRL_MESSAGE_OP_HID_REPORT,
    NL_CTRL_MESSAGE_OP_START_USB_DEVICE,
} node_link_ctrl_msg_opcode_t;

typedef enum {
    NL_NODE_MESSAGE_OP_INIT,
    NL_NODE_MESSAGE_OP_SET_REPORT,
    NL_NODE_MESSAGE_OP_SET_HID_PROTOCOL,
} node_link_node_msg_opcode_t;

typedef struct __attribute__((packed)) {
    uint8_t header; // Always 0x42
    uint8_t protocol_opcode; //
    uint32_t message_len;
    uint16_t message_crc;
    // Keep header_crc at the end of the struct
    uint16_t header_crc;
} node_link_transport_header_t;

// This allow to send larger chunk of data that are malloced
typedef struct __attribute__((packed)) {
    uint16_t data_len;
    uint8_t *data;
    bool should_free_once_sent;
} node_link_msg_extra_data_t;

typedef struct __attribute__((packed)) {
    uint8_t opcode; /**< \see node_link_ctrl_msg_opcode_t and \see node_link_node_msg_opcode_t */
    uint16_t data_len;
    uint8_t extra_data_count;
} nl_msg_header_t;

typedef struct __attribute__((packed)) {
    nl_msg_header_t header;
    node_link_msg_extra_data_t extra_data[1]; // For now only 1 extra data is possible
    uint8_t data[128];
} node_link_msg_t;

// ┌──────────────────────────────────┐
// │        Controller -> Node        │
// └──────────────────────────────────┘

typedef struct __attribute__((packed)) {
    uint8_t computer_id;
} nl_ctrl_msg_init_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
    const uint8_t kvm_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
    const uint8_t itf_protocol; /**< Interface protocol \see hid_interface_protocol_enum_t */
    const uint16_t vid; /**< The USB vendor id */
    const uint16_t pid; /**< The USB product id */
    /** The report data is sent as extra data, size is variable to this one is malloced
     * const uint16_t desc_len; -- HID report descriptor length
     * uint8_t *report_desc; -- HID report descriptor. (malloced, need to be freed by consumer). \see hid1_11.pdf
     */
} nl_ctrl_msg_hid_mount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t dev_addr; /**< Device address (Which port the device is connected. Values: 1, 2) */
    const uint8_t host_hid_idx; /**< HID interface index on USB Host side (Where the keyboard / mouse are connected) */
} nl_ctrl_msg_hid_umount_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t kvm_hid_idx; /**< The index of the HID interface in the KVM (Exposed to the computers) */
    const uint8_t report_id; /**< The report ID if any. 0 = no report id */
    const uint16_t report_data_len; /**< The length of the report data */
    uint8_t report_data[96]; /**< The report data */
} nl_ctrl_msg_hid_report_data_t;

typedef struct __attribute__((packed)) {
    const uint16_t vid;
    const uint16_t pid;
} nl_ctrl_msg_start_usb_device_data_t;

// ┌──────────────────────────────────┐
// │        Node -> Controller        │
// └──────────────────────────────────┘

typedef struct __attribute__((packed)) {
} nl_node_msg_node_init_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t kvm_hid_idx; /**< The index of the HID interface in the KVM (Exposed to the computers) */
    const uint8_t report_id; /**< The report ID if any. 0 = no report id */
    const uint8_t report_type; /**< The report type \see hid_report_type_t */
    /* The report data is sent as extra data, size is variable to this one is malloced*/
    // const uint16_t report_data_len; /**< The length of the report data */
    // uint8_t *report_data; /**< The report data */
} nl_node_msg_hid_set_report_data_t;

typedef struct __attribute__((packed)) {
    const uint8_t kvm_hid_idx; /**< The index of the HID interface in the KVM (Exposed to the computers) */
    const uint8_t hid_protocol; /**< HID protocol (boot / report) \see hid_protocol_mode_enum_t */
} nl_node_msg_hid_set_hid_protocol_data_t;


// ╔══════════════════════════════════╗
// ║               Logic              ║
// ╚══════════════════════════════════╝

typedef void (*message_handler_t)(const node_link_msg_t *message, void *udata);

typedef struct {
    bool is_controller;
    spi_inst_t *spi;

    uint8_t spi_select_gpio;
    uint8_t spi_ready_gpio;

    /**
     * We need a buffer large enough to a full report_descriptor that can be a maximum theoretically of 65536 bytes
     * This is unlikely to be reached in practice, but it's a safe upper bound, if we need more memory at some point
     * this can probably be reduced
     * For now going with 4Kb
     */
    uint8_t rx_buffer[4096 + 256];
    uint8_t tx_buffer[4096 + 256];
    uint8_t drain_buffer[100];
    message_handler_t message_handler;
    size_t baud_rate;
} node_link_t;

void node_link_init_controller(
    node_link_t *link,
    spi_inst_t *spi,
    message_handler_t message_handler
);

void node_link_init_node(
    node_link_t *link,
    spi_inst_t *spi,
    uint8_t spi_ready_gpio,
    message_handler_t message_handler
);

void node_link_spi_select_target(
    node_link_t *link,
    uint8_t spi_ready_gpio,
    uint8_t spi_select_gpio
);

bool node_link_send_message_blocking(
    node_link_t *link,
    const node_link_msg_t *tx_message,
    void *udata
);

void node_link_drain_buffer(
    const node_link_t *link
);

void node_link_drain_rx(
    const node_link_t *link
);

uint64_t node_link_get_us_delay_before_retry(
    const node_link_t *link
);

void node_link_dispose_message(
    const node_link_msg_t *message
);