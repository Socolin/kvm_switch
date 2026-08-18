#ifndef CH9350L_H
#define CH9350L_H

#include "pico/stdlib.h"

// ╔══════════════════════════════════╗
// ║             Frames               ║
// ╚══════════════════════════════════╝

// ┌──────────────────────────────────┐
// │   Device Connection Frame        │
// └──────────────────────────────────┘

// Not needed so far

// 0x57 0xAB 0x81 1-byte ID 2-byte Payload length Payload 2-byte ID 1-byte parity check

// ┌──────────────────────────────────┐
// │  Status Request Frame            │
// └──────────────────────────────────┘

// 0x57 0xAB 0x82 0xA*
// Response: 0x57 0xAB 0x12 0x00 0x00 0x00 0x00 0xFF 0x80 0x00 0x20
// Response: 0x57 0xAB 0x12 0x00 0x00 0x00 0x00 0xFF 0xFF 0x00 0x20

typedef struct {
    uint8_t io_status_value;
} ch9350l_message_status_request_frame_t;

// ┌──────────────────────────────────┐
// │   Reset Delay Command            │
// └──────────────────────────────────┘

// 0x57 0xAB 0x84

// ┌──────────────────────────────────┐
// │  Working Status Change Command   │
// └──────────────────────────────────┘

// 0x57 0xAB 0x80 1-byte status value

typedef struct {
    uint8_t status_value;
} ch9350l_message_working_status_change_command_t;

// ┌──────────────────────────────────┐
// │  Working Status Change Command 1 │
// └──────────────────────────────────┘

// 0x57 0xAB 0x85 1-byte status value
//   0x02: The working status of upper computer will switch to state 2;
//   0x03: The working status of the upper computer will switch to state 3.

typedef struct {
    uint8_t status_value;
} ch9350l_message_working_status_change_command_1_t;

// ┌──────────────────────────────────┐
// │  Working Status Change Command 2 │
// └──────────────────────────────────┘

// 0x57 0xAB 0x40 1-byte status value

typedef struct {
    uint8_t status_value;
} ch9350l_message_working_status_change_command_2_t;

// ┌──────────────────────────────────┐
// │   Device Disconnect Command      │
// └──────────────────────────────────┘

// 0x57 0xAB 0x86

// ┌──────────────────────────────────┐
// │ Obtain Version Number Command    │
// └──────────────────────────────────┘

// 0x57 0xAB 0x87

// ┌──────────────────────────────────┐
// │ Valid Key Value Frame            │
// └──────────────────────────────────┘

// 0x57 0xAB 0x83/0x88

typedef enum {
    CH9350L_VALID_KEY_PROTOCOL_UNKNOWN,
    CH9350L_VALID_KEY_PROTOCOL_HID,
    CH9350L_VALID_KEY_PROTOCOL_BIOS,
    CH9350L_VALID_KEY_PROTOCOL_RESERVED,
} ch9350l_valid_key_protocol_t;

typedef enum {
    CH9350L_VALID_KEY_KIND_OTHER,
    CH9350L_VALID_KEY_KIND_KEYBOARD,
    CH9350L_VALID_KEY_KIND_MOUSE,
    CH9350L_VALID_KEY_KIND_MULTIMEDIA,
} ch9350l_valid_key_kind_t;

typedef struct {
    uint8_t port;
    ch9350l_valid_key_protocol_t protocol;
    ch9350l_valid_key_kind_t kind;
} ch9350l_valid_key_value_labeling_t;

// > Valid key value frame: The data length is less than 72 bytes.
#define CH9350L_KEY_VALUE_DATA_MAX_LEN 72

typedef struct {
    ch9350l_valid_key_value_labeling_t labeling;
    uint8_t serial_no;
    uint8_t check;
    uint8_t key_value_data[CH9350L_KEY_VALUE_DATA_MAX_LEN];
    uint8_t key_value_data_length;
} ch9350l_valid_key_value_frame_t;

// ╔══════════════════════════════════╗
// ║             Messages             ║
// ╚══════════════════════════════════╝

typedef enum {
    CH9350_MESSAGE_TYPE_STATUS_REQUEST_FRAME,
    CH9350_MESSAGE_TYPE_VALID_KEY_VALUE_FRAME,
    CH9350_MESSAGE_TYPE_RESET_DELAY_COMMAND,
    CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND,
    CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND_1,
    CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND_2,
    CH9350_MESSAGE_TYPE_OBTAIN_VERSION_NUMBER_COMMAND,
    CH9350_MESSAGE_TYPE_DEVICE_DISCONNECT_COMMAND,
} ch9350l_message_type_t;

typedef struct {
    ch9350l_message_type_t type;
    uint8_t data[CH9350L_KEY_VALUE_DATA_MAX_LEN + 8];
    uint8_t data_length;
} ch9350l_message_t;

// ╔══════════════════════════════════╗
// ║               Core               ║
// ╚══════════════════════════════════╝

#define CH9350L_RECEIVING_BUFFER_MAX_LEN 128
#define CH9350L_MESSAGE_QUEUE_SIZE 32

typedef struct ch9350l ch9350l_t;

typedef void (*ch9350l_send_data_func_t)(
    const ch9350l_t *ch9350l,
    const uint8_t *data,
    size_t len
);

typedef struct ch9350l {
    uint16_t buffer_count;
    uint8_t buffer[CH9350L_RECEIVING_BUFFER_MAX_LEN];
    ch9350l_message_t *messages[CH9350L_MESSAGE_QUEUE_SIZE];
    ch9350l_message_t messages_pool[CH9350L_MESSAGE_QUEUE_SIZE];
    uint8_t messages_from_pool_in_use[CH9350L_MESSAGE_QUEUE_SIZE];
    uint8_t port1_hid;
    uint8_t port2_hid;
    uint8_t keyboard_report_value;
    uint8_t current_status; // 0x80 = state 0, 0x00 = state 1
    uint8_t status_value;
    volatile uint8_t messages_head;
    volatile uint8_t messages_tail;
    void* user_data;
    ch9350l_send_data_func_t send_data_func;
} ch9350l_t;

/**
 * Initialize a new ch9350l device on a default uart of the pico pi, either uart0 or uart1
 * @param uart the default uart to use
 * @param gpio_tx The tx pin (Connected to the RX pin of the ch9350l) Must be a valid pin for the given uart
 * @param gpio_rx The rx pin (Connected to the TX pin of the ch9350l) Must be a valid pin for the given uart
 * @param baudrate The baudrate to use, see BA0 and BA1 on the ch9350l
 * @return a pointer to the initialized ch9350l device
 */
ch9350l_t *ch9350l_init_with_builtin_uart(
    uart_inst_t *uart,
    uint gpio_tx,
    uint gpio_rx,
    uint baudrate
);

// ╔══════════════════════════════════╗
// ║           Functions              ║
// ╚══════════════════════════════════╝

typedef enum {
    CH9350_READ_RESULT_NO_MESSAGE,
    CH9350_READ_RESULT_MESSAGE_AVAILABLE,
} ch9350l_read_result_t;

void ch9350l_init(
    const ch9350l_t *ch9350l
);

// ┌──────────────────────────────────┐
// │             Messages             │
// └──────────────────────────────────┘

ch9350l_message_t *ch9350l_dequeue_message(
    ch9350l_t *ch9350l
);

void ch9350l_message_free(
    ch9350l_t *ch9350l,
    ch9350l_message_t *message
);

// ┌──────────────────────────────────┐
// │               Core               │
// └──────────────────────────────────┘

void ch9350l_received_byte(
    ch9350l_t *ch9350l,
    uint8_t c
);

void ch9350l_set_to_state_1(
    ch9350l_t *ch9350l
);

void ch9350l_set_to_state_0(
    ch9350l_t *ch9350l
);

void ch9350l_set_keyboard_report(
    ch9350l_t *ch9350l,
    int keyboard_report_value
);

// ┌──────────────────────────────────┐
// │               Debug              │
// └──────────────────────────────────┘

void ch9350l_debug_print_message(
    const ch9350l_message_t *message
);

const char *ch9350l_debug_protocol_to_string(
    ch9350l_valid_key_protocol_t protocol
);

const char *ch9350l_debug_kind_to_string(
    ch9350l_valid_key_kind_t kind
);
#endif
