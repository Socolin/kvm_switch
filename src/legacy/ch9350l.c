#include <string.h>

#include "../../ch9350l.h"

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"

// ╔══════════════════════════════════╗
// ║             Messages             ║
// ╚══════════════════════════════════╝

static ch9350l_message_t *ch9350l_message_new(
    ch9350l_t *ch9350l,
    ch9350l_message_type_t type
) {
    for (int i = 0; i < CH9350L_MESSAGE_QUEUE_SIZE; i++) {
        if (ch9350l->messages_from_pool_in_use[i] == 0) {
            ch9350l->messages_from_pool_in_use[i] = 1;
            ch9350l_message_t *message = &ch9350l->messages_pool[i];
            memset(message, 0, sizeof(ch9350l_message_t));
            message->type = type;
            return message;
        }
    }
    return NULL;
}

void ch9350l_message_free(
    ch9350l_t *ch9350l,
    ch9350l_message_t *message
    ) {
    for (int i = 0; i < CH9350L_MESSAGE_QUEUE_SIZE; i++) {
        if (message == &ch9350l->messages_pool[i]) {
            ch9350l->messages_from_pool_in_use[i] = 0;
            return;
        }
    }
    free(message);
}

static int ch9350l_enqueue_message(
    ch9350l_t *ch9350l,
    ch9350l_message_t *message
) {
    const int head = ch9350l->messages_head;
    const int next_head = (head + 1) % CH9350L_MESSAGE_QUEUE_SIZE;

    if (next_head == ch9350l->messages_tail) {
        printf("[Warning][CH9350] msg queue is full\n");
        return 0;
    }

    ch9350l->messages[head] = message;
    ch9350l->messages_head = next_head;
    return 1;
}

ch9350l_message_t *ch9350l_dequeue_message(
    ch9350l_t *ch9350l
) {
    const int tail = ch9350l->messages_tail;
    if (tail == ch9350l->messages_head) {
        return NULL;
    }

    ch9350l_message_t *message = ch9350l->messages[tail];
    ch9350l->messages[tail] = NULL;
    ch9350l->messages_tail = (tail + 1) % CH9350L_MESSAGE_QUEUE_SIZE;

    return message;
}

// ╔══════════════════════════════════╗
// ║             Decoding             ║
// ╚══════════════════════════════════╝

typedef enum {
    CH9350L_DECODE_NOT_ENOUGH_DATA,
    CH9350L_DECODE_MALFORMED_DATA,
    CH9350L_DECODE_OUT_OF_MEMORY,
    CH9350L_DECODE_RECEIVING_BUFFER_TOO_SMALL,
    CH9350L_DECODE_KEY_VALUE_DATA_TOO_BIG,
    CH9350L_DECODE_CHECKSUM_MISMATCH,
    CH9350L_DECODE_DECODE_SUCCESS,
} ch9350l_decode_result_t;

ch9350l_decode_result_t ch9350l_try_decode_message(
    ch9350l_t *ch9350l,
    ch9350l_message_t **out_message
) {
    *out_message = NULL;

    if (ch9350l->buffer_count < 3)
        return CH9350L_DECODE_NOT_ENOUGH_DATA;

    // Try to read the header
    if (ch9350l->buffer[0] != 0x57 || ch9350l->buffer[1] != 0xab)
        return CH9350L_DECODE_MALFORMED_DATA;

    // Decoding: Status Request Frame
    if (ch9350l->buffer[2] == 0x82) {
        if (ch9350l->buffer_count < 4) {
            return CH9350L_DECODE_NOT_ENOUGH_DATA; // Not enough data
        }

        if ((ch9350l->buffer[3] & 0xA0) != 0xA0) {
            return CH9350L_DECODE_MALFORMED_DATA;
        }

        ch9350l_message_t *message = ch9350l_message_new(ch9350l, CH9350_MESSAGE_TYPE_STATUS_REQUEST_FRAME);
        if (message == NULL) {
            return CH9350L_DECODE_OUT_OF_MEMORY;
        }

        ch9350l_message_status_request_frame_t *message_data = (ch9350l_message_status_request_frame_t *) message->data;
        message_data->io_status_value = ch9350l->buffer[3] & 0xf;
        message->data_length = 1;
        *out_message = message;
        return CH9350L_DECODE_DECODE_SUCCESS;
    }

    // Decoding: Valid Key Value Frame
    if (ch9350l->buffer[2] == 0x83 || ch9350l->buffer[2] == 0x88) {
        if (ch9350l->buffer_count < 4) {
            return CH9350L_DECODE_NOT_ENOUGH_DATA; // Not enough data
        }
        const uint16_t len = ch9350l->buffer[3];
        if (ch9350l->buffer_count < len + 4) {
            return CH9350L_DECODE_NOT_ENOUGH_DATA; // Not enough data
        }
        if (len > sizeof(ch9350l->buffer) - 4) {
            // Receiving buffer is too small
            return CH9350L_DECODE_RECEIVING_BUFFER_TOO_SMALL;
        }
        uint8_t key_value_data_length = len - 3;
        if (key_value_data_length > CH9350L_KEY_VALUE_DATA_MAX_LEN) {
            return CH9350L_DECODE_KEY_VALUE_DATA_TOO_BIG;
        }

        uint8_t serial_no = ch9350l->buffer[5 + key_value_data_length];
        uint8_t check = ch9350l->buffer[6 + key_value_data_length];
        uint8_t checksum = 0;
        checksum += serial_no;
        for (uint8_t i = 0; i < key_value_data_length; i++) {
            checksum += ch9350l->buffer[i + 5];
        }

        if (checksum != check)
            return CH9350L_DECODE_CHECKSUM_MISMATCH;

        ch9350l_message_t *message = ch9350l_message_new(ch9350l, CH9350_MESSAGE_TYPE_VALID_KEY_VALUE_FRAME);
        if (message == NULL) {
            return CH9350L_DECODE_OUT_OF_MEMORY;
        }
        *out_message = message;

        const uint8_t labelling = ch9350l->buffer[4];
        ch9350l_valid_key_value_frame_t *message_data = ((ch9350l_valid_key_value_frame_t *) message->data);
        message_data->labeling.port = labelling & 0x1;
        message_data->labeling.protocol = (labelling >> 1) & 0x3;
        message_data->labeling.kind = (labelling >> 4) & 0x3;
        message_data->key_value_data_length = key_value_data_length;
        memcpy(message_data->key_value_data, ch9350l->buffer + 5, key_value_data_length);
        message_data->serial_no = serial_no;
        message_data->check = check;

        return CH9350L_DECODE_DECODE_SUCCESS;
    }

    // Decoding: Reset Delay Command
    if (ch9350l->buffer[2] == 0x84) {
        ch9350l_message_t *message = ch9350l_message_new(ch9350l, CH9350_MESSAGE_TYPE_RESET_DELAY_COMMAND);
        if (message == NULL) {
            return CH9350L_DECODE_OUT_OF_MEMORY;
        }
        *out_message = message;
        return CH9350L_DECODE_DECODE_SUCCESS;
    }

    // Decoding: Working Status Change Command 1
    if (ch9350l->buffer[2] == 0x85) {
        if (ch9350l->buffer_count < 4) {
            return CH9350L_DECODE_NOT_ENOUGH_DATA; // Not enough data
        }

        ch9350l_message_t *message = ch9350l_message_new(ch9350l, CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND_1);
        if (message == NULL) {
            return CH9350L_DECODE_OUT_OF_MEMORY;
        }
        *out_message = message;
        ch9350l_message_working_status_change_command_1_t *message_data = (
            ch9350l_message_working_status_change_command_1_t *) message->data;
        message_data->status_value = ch9350l->buffer[3];
        message->data_length = 1;
        return CH9350L_DECODE_DECODE_SUCCESS;
    }

    // Decoding: Device Disconnect Command
    if (ch9350l->buffer[2] == 0x86) {
        ch9350l_message_t *message = ch9350l_message_new(ch9350l, CH9350_MESSAGE_TYPE_DEVICE_DISCONNECT_COMMAND);
        if (message == NULL) {
            return CH9350L_DECODE_OUT_OF_MEMORY;
        }
        *out_message = message;
        return CH9350L_DECODE_DECODE_SUCCESS;
    }

    // Decoding: Obtain Version Number Command
    if (ch9350l->buffer[2] == 0x87) {
        ch9350l_message_t *message = ch9350l_message_new(ch9350l, CH9350_MESSAGE_TYPE_OBTAIN_VERSION_NUMBER_COMMAND);
        if (message == NULL) {
            return CH9350L_DECODE_OUT_OF_MEMORY;
        }
        *out_message = message;
        return CH9350L_DECODE_DECODE_SUCCESS;
    }

    // Decoding: Working Status Change Command 2
    if (ch9350l->buffer[2] == 0x40) {
        if (ch9350l->buffer_count < 4) {
            return CH9350L_DECODE_NOT_ENOUGH_DATA; // Not enough data
        }

        ch9350l_message_t *message = ch9350l_message_new(ch9350l, CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND_2);
        if (message == NULL) {
            return CH9350L_DECODE_OUT_OF_MEMORY;
        }
        *out_message = message;
        ch9350l_message_working_status_change_command_2_t *message_data = (
            ch9350l_message_working_status_change_command_2_t *) message->data;
        message_data->status_value = ch9350l->buffer[3];
        message->data_length = 1;
        return CH9350L_DECODE_DECODE_SUCCESS;
    }

    // Decoding: Working Status Change Command
    if (ch9350l->buffer[2] == 0x80) {
        if (ch9350l->buffer_count < 4) {
            return CH9350L_DECODE_NOT_ENOUGH_DATA; // Not enough data
        }

        ch9350l_message_t *message = ch9350l_message_new(ch9350l, CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND);
        if (message == NULL) {
            return CH9350L_DECODE_OUT_OF_MEMORY;
        }
        *out_message = message;
        ch9350l_message_working_status_change_command_t *message_data = (
            ch9350l_message_working_status_change_command_t *) message->data;
        message_data->status_value = ch9350l->buffer[3];
        message->data_length = 1;
        return CH9350L_DECODE_DECODE_SUCCESS;
    }

    return CH9350L_DECODE_MALFORMED_DATA;
}

const char *ch9350l_debug_protocol_to_string(
    const ch9350l_valid_key_protocol_t protocol
) {
    switch (protocol) {
        case CH9350L_VALID_KEY_PROTOCOL_HID:
            return "hid";
        case CH9350L_VALID_KEY_PROTOCOL_BIOS:
            return "bios";
        case CH9350L_VALID_KEY_PROTOCOL_UNKNOWN:
            return "unknown";
        case CH9350L_VALID_KEY_PROTOCOL_RESERVED:
            return "reserved";
        default:
            return "invalid";
    }
}

const char *ch9350l_debug_kind_to_string(
    const ch9350l_valid_key_kind_t kind
) {
    switch (kind) {
        case CH9350L_VALID_KEY_KIND_KEYBOARD:
            return "keyboard";
        case CH9350L_VALID_KEY_KIND_MOUSE:
            return "mouse";
        case CH9350L_VALID_KEY_KIND_MULTIMEDIA:
            return "multimedia";
        case CH9350L_VALID_KEY_KIND_OTHER:
            return "other";
        default:
            return "invalid";
    }
}

const char *ch9350l_debug_message_type_to_string(
    const ch9350l_message_type_t kind
) {
    switch (kind) {
        case CH9350_MESSAGE_TYPE_VALID_KEY_VALUE_FRAME:
            return "Valid Key Value Frame";
        case CH9350_MESSAGE_TYPE_RESET_DELAY_COMMAND:
            return "Reset Delay Command";
        case CH9350_MESSAGE_TYPE_STATUS_REQUEST_FRAME:
            return "Status Request Frame";
        case CH9350_MESSAGE_TYPE_DEVICE_DISCONNECT_COMMAND:
            return "Device Disconnect Command";
        case CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND:
            return "Working Status Change Command";
        case CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND_1:
            return "Working Status Change Command 1";
        case CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND_2:
            return "Working Status Change Command 2";
        case CH9350_MESSAGE_TYPE_OBTAIN_VERSION_NUMBER_COMMAND:
            return "Obtain Version Number Command";
        default:
            return "invalid";
    }
}

void ch9350l_debug_print_message(
    const ch9350l_message_t *message
) {
    printf("[Debug][CH9350] Message: %d (%s)\n", message->type,
           ch9350l_debug_message_type_to_string(message->type));
    switch (message->type) {
        case CH9350_MESSAGE_TYPE_STATUS_REQUEST_FRAME: {
            const ch9350l_message_status_request_frame_t *msg_data
                    = (ch9350l_message_status_request_frame_t *) message->data;
            printf("  io_status_value: %d\n", msg_data->io_status_value);
            break;
        }
        case CH9350_MESSAGE_TYPE_VALID_KEY_VALUE_FRAME: {
            const ch9350l_valid_key_value_frame_t *msg_data
                    = (ch9350l_valid_key_value_frame_t *) message->data;

            printf("  labeling.kind: %d (%s)\n", msg_data->labeling.kind,
                   ch9350l_debug_kind_to_string(msg_data->labeling.kind));
            printf("  labeling.protocol: %d (%s)\n", msg_data->labeling.protocol,
                   ch9350l_debug_protocol_to_string(msg_data->labeling.protocol));
            printf("  labeling.port: %d\n", msg_data->labeling.port);
            printf("  serial_no: %d\n", msg_data->serial_no);
            printf("  check: %d\n", msg_data->check);
            printf("  key_value_data_len: %d\n", msg_data->key_value_data_length);
            printf("  key_value_data: ");
            for (int i = 0; i < msg_data->key_value_data_length; i++) {
                printf("%02x  ", msg_data->key_value_data[i]);
            }
            printf("\n");
            break;
        }
        case CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND: {
            const ch9350l_message_working_status_change_command_t *msg_data
                    = (ch9350l_message_working_status_change_command_t *) message->data;
            printf("  status_value: %d\n", msg_data->status_value);
            break;
        }
        case CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND_1: {
            const ch9350l_message_working_status_change_command_1_t *msg_data
                    = (ch9350l_message_working_status_change_command_1_t *) message->data;
            printf("  status_value: %d\n", msg_data->status_value);
            break;
        }
        case CH9350_MESSAGE_TYPE_WORKING_STATUS_CHANGE_COMMAND_2: {
            const ch9350l_message_working_status_change_command_2_t *msg_data
                    = (ch9350l_message_working_status_change_command_2_t *) message->data;
            printf("  status_value: %d\n", msg_data->status_value);
            break;
        }
        case CH9350_MESSAGE_TYPE_RESET_DELAY_COMMAND:
        case CH9350_MESSAGE_TYPE_OBTAIN_VERSION_NUMBER_COMMAND:
        case CH9350_MESSAGE_TYPE_DEVICE_DISCONNECT_COMMAND: {
            break;
        }

        default:
            printf("[Error] Unknown message type: %d\n", message->type);
            break;
    }
}

void ch9350l_debug_print_buffer(const ch9350l_t *ch9350l) {
    for (int i = 0; i < ch9350l->buffer_count; i++) {
        printf("%02x  ", ch9350l->buffer[i]);
    }
    printf("\n");
}

void ch9350l_drop_first_byte(ch9350l_t *ch9350l) {
    if (ch9350l->buffer_count == 0)
        return;

    ch9350l->buffer_count--;
    for (int i = 0; i < ch9350l->buffer_count; i++) {
        ch9350l->buffer[i] = ch9350l->buffer[i + 1];
    }
}

// ╔══════════════════════════════════╗
// ║               Core               ║
// ╚══════════════════════════════════╝

void ch9350l_received_byte(
    ch9350l_t *ch9350l,
    const uint8_t c
) {
    if (ch9350l->buffer_count >= CH9350L_RECEIVING_BUFFER_MAX_LEN) {
        printf("[Warning][CH9350] CH9350 buffer is full but no packet was successfully detected, dropping it: ");
        ch9350l_debug_print_buffer(ch9350l);
        ch9350l->buffer_count = 0;
    }

    ch9350l->buffer[ch9350l->buffer_count++] = c;

    ch9350l_decode_result_t result;
    ch9350l_message_t *message = NULL;
retry:
    result = ch9350l_try_decode_message(ch9350l, &message);
    switch (result) {
        case CH9350L_DECODE_DECODE_SUCCESS:
            // printf("[Debug][CH9350] Packet received\n");
            ch9350l->buffer_count = 0;
            if (!ch9350l_enqueue_message(ch9350l, message)) {
                printf("[Warning][CH9350] CH9350 message queue is full, dropping message");
                ch9350l_message_free(ch9350l, message);
            }
            return;
        case CH9350L_DECODE_NOT_ENOUGH_DATA:
            // printf("[Debug][CH9350] Not enough data\n");
            break;
        case CH9350L_DECODE_MALFORMED_DATA:
            printf("[Warning][CH9350] Malformed data when decoding frame. Dropping first byte:\n");
            ch9350l_debug_print_buffer(ch9350l);
            ch9350l_drop_first_byte(ch9350l);
            goto retry;
            break;
        case CH9350L_DECODE_RECEIVING_BUFFER_TOO_SMALL:
            printf("[Warning][CH9350] Error when decoding frame. Buffer too small:\n");
            ch9350l_debug_print_buffer(ch9350l);
            ch9350l->buffer_count = 0;
            break;
        case CH9350L_DECODE_KEY_VALUE_DATA_TOO_BIG:
            printf("[Warning][CH9350] Malformed data when decoding frame. Key value too big:\n");
            ch9350l_debug_print_buffer(ch9350l);
            ch9350l->buffer_count = 0;
            break;
        case CH9350L_DECODE_CHECKSUM_MISMATCH:
            printf("[Warning][CH9350] Malformed data when decoding frame. Invalid checksum:\n");
            ch9350l_debug_print_buffer(ch9350l);
            ch9350l->buffer_count = 0;
            break;
        default:
            break;
    }
}

static void ch9350l_send_specific_data_frame(
    const ch9350l_t *ch9350l
) {
    static uint8_t frame_data[] =
    {
        0x57, 0xAB, 0x12,
        0x00, // [3] PID port 1
        0x00, // [4] PID port 1
        0x00, // [5] PID port 2
        0x00, // [6] PID port 2
        0xFF, // [7] Keyboard report value
        0xFF, // [8] Current Status
        0xFF, // [9] Status value
        0x20, // [10] end
    };

    frame_data[3] = ch9350l->port1_hid & 0xff;
    frame_data[4] = (ch9350l->port1_hid >> 8) & 0xff;
    frame_data[5] = ch9350l->port2_hid & 0xff;
    frame_data[6] = (ch9350l->port2_hid >> 8) & 0xff;
    frame_data[7] = ch9350l->keyboard_report_value;
    frame_data[8] = ch9350l->current_status;
    frame_data[9] = ch9350l->status_value;

    ch9350l->send_data_func(ch9350l, frame_data, sizeof(frame_data));
}

void ch9350l_set_to_state_1(
    ch9350l_t *ch9350l
) {
    ch9350l->current_status = 0xFF;
    ch9350l_send_specific_data_frame(ch9350l);
}

void ch9350l_set_to_state_0(
    ch9350l_t *ch9350l
) {
    ch9350l->current_status = 0x80;
    ch9350l_send_specific_data_frame(ch9350l);
}

void ch9350l_set_keyboard_report(
    ch9350l_t *ch9350l,
    int keyboard_report_value
) {
    ch9350l->keyboard_report_value = keyboard_report_value;
    ch9350l_send_specific_data_frame(ch9350l);
}

static ch9350l_t ch9350l_uart0 = {0};
static ch9350l_t ch9350l_uart1 = {0};

static void ch9350l_on_uart1_rx() {
    while (uart_is_readable(uart1)) {
        ch9350l_received_byte(&ch9350l_uart1, uart_getc(uart1));
    }
}

static void ch9350l_on_uart0_rx() {
    while (uart_is_readable(uart0)) {
        ch9350l_received_byte(&ch9350l_uart0, uart_getc(uart0));
    }
}

static void ch9350l_send_data_with_builtin_uart(
    const ch9350l_t *ch9350l,
    const uint8_t *data,
    const size_t len
) {
    uart_write_blocking(ch9350l->user_data, data, len);
}

ch9350l_t *ch9350l_init_with_builtin_uart(
    uart_inst_t *uart,
    uint gpio_tx,
    uint gpio_rx,
    uint baudrate
) {
    uart_init(uart, baudrate);
    gpio_set_function(gpio_tx, GPIO_FUNC_UART);
    gpio_set_function(gpio_rx, GPIO_FUNC_UART);

    if (uart == uart0) {
        irq_set_exclusive_handler(UART0_IRQ, ch9350l_on_uart0_rx);
        irq_set_enabled(UART0_IRQ, true);
        uart_set_irq_enables(uart0, true, false);
        ch9350l_uart0.status_value = 0xAC;
        ch9350l_uart0.user_data = uart0;
        ch9350l_uart0.send_data_func = ch9350l_send_data_with_builtin_uart;
        ch9350l_set_to_state_0(&ch9350l_uart0);
        return &ch9350l_uart0;
    }

    if (uart == uart1) {
        irq_set_exclusive_handler(UART1_IRQ, ch9350l_on_uart1_rx);
        irq_set_enabled(UART1_IRQ, true);
        uart_set_irq_enables(uart1, true, false);
        ch9350l_uart1.status_value = 0xAC;
        ch9350l_uart1.user_data = uart1;
        ch9350l_uart1.send_data_func = ch9350l_send_data_with_builtin_uart;
        ch9350l_set_to_state_0(&ch9350l_uart1);
        return &ch9350l_uart1;
    }

    return NULL;
}
