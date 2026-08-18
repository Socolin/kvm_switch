#include "../../ch9329.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <machine/endian.h>

#include "../../debug.h"

// ╔══════════════════════════════════╗
// ║              Frame               ║
// ╚══════════════════════════════════╝

static ch9329_frame_t *ch9329_frame_new(
    ch9329_t *ch9329
) {
    for (int i = 0; i < CH9329_MESSAGE_QUEUE_SIZE; i++) {
        if (ch9329->frames_from_pool_in_use[i] == 0) {
            ch9329->frames_from_pool_in_use[i] = 1;
            ch9329_frame_t *frame = &ch9329->frames_pool[i];
            memset(frame, 0, sizeof(ch9329_frame_t));
            return frame;
        }
    }
    return NULL;
}

void ch9329_frame_free(
    ch9329_t *ch9329,
    ch9329_frame_t *frame
) {
    for (int i = 0; i < CH9329_MESSAGE_QUEUE_SIZE; i++) {
        if (frame == &ch9329->frames_pool[i]) {
            ch9329->frames_from_pool_in_use[i] = 0;
            return;
        }
    }
    free(frame);
}

static int ch9329_enqueue_frame(
    ch9329_t *ch9329,
    ch9329_frame_t *frame
) {
    const int head = ch9329->frames_head;
    const int next_head = (head + 1) % CH9329_MESSAGE_QUEUE_SIZE;

    if (next_head == ch9329->frames_tail) {
        printf("[Warning][CH9329] msg queue is full\n");
        return 0;
    }

    ch9329->frames[head] = frame;
    ch9329->frames_head = next_head;
    return 1;
}

ch9329_frame_t *ch9329_dequeue_frame(
    ch9329_t *ch9329
) {
    const int tail = ch9329->frames_tail;
    if (tail == ch9329->frames_head) {
        return NULL;
    }

    ch9329_frame_t *frame = ch9329->frames[tail];
    ch9329->frames[tail] = NULL;
    ch9329->frames_tail = (tail + 1) % CH9329_MESSAGE_QUEUE_SIZE;

    return frame;
}

// ╔══════════════════════════════════╗
// ║             Decoding             ║
// ╚══════════════════════════════════╝

typedef enum {
    CH9329L_DECODE_NOT_ENOUGH_DATA,
    CH9329L_DECODE_MALFORMED_DATA,
    CH9329L_DECODE_OUT_OF_MEMORY,
    CH9329L_DECODE_NO_PACKET_AVAILABLE,
    CH9329L_DECODE_CHECKSUM_MISMATCH,
    CH9329L_DECODE_SUCCESS,
} ch9329_decode_result_t;

static ch9329_decode_result_t ch9329_try_decode_frame(
    ch9329_t *ch9329,
    ch9329_frame_t **out_frame
) {
    *out_frame = NULL;

    // All packet are at least 6 bytes long
    if (ch9329->buffer_count < 6)
        return CH9329L_DECODE_NOT_ENOUGH_DATA;

    const size_t data_len = ch9329->buffer[4];
    if (data_len > CH9329_MAX_PACKET_DATA_LEN)
        return CH9329L_DECODE_MALFORMED_DATA;
    if (ch9329->buffer_count < data_len + 6)
        return CH9329L_DECODE_NOT_ENOUGH_DATA;

    // Try to read the header, always 0x57 0xAB
    if (ch9329->buffer[0] != 0x57 || ch9329->buffer[1] != 0xab)
        return CH9329L_DECODE_MALFORMED_DATA;

    uint8_t checksum = ch9329->buffer[5 + data_len];
    uint8_t calculated_checksum = 0;
    for (size_t i = 0; i < 5 + data_len; i++) {
        calculated_checksum += ch9329->buffer[i];
    }
    if (calculated_checksum != checksum)
        return CH9329L_DECODE_CHECKSUM_MISMATCH;

    ch9329_frame_t *frame = ch9329_frame_new(ch9329);
    if (frame == NULL)
        return CH9329L_DECODE_NO_PACKET_AVAILABLE;

    frame->address = ch9329->buffer[2];
    frame->command = ch9329->buffer[3];
    frame->data_length = data_len;
    for (size_t i = 0; i < data_len; i++) {
        frame->data[i] = ch9329->buffer[5 + i];
    }
    frame->checksum = checksum;

    *out_frame = frame;

    return CH9329L_DECODE_SUCCESS;
}

void ch9329_drop_first_byte(ch9329_t *ch9329) {
    if (ch9329->buffer_count == 0)
        return;

    ch9329->buffer_count--;
    for (int i = 0; i < ch9329->buffer_count; i++) {
        ch9329->buffer[i] = ch9329->buffer[i + 1];
    }
}

// ╔══════════════════════════════════╗
// ║               Core               ║
// ╚══════════════════════════════════╝

void ch9329_received_byte(
    ch9329_t *ch9329,
    const uint8_t c
) {
    if (ch9329->buffer_count >= CH9329_RECEIVING_BUFFER_MAX_LEN) {
        printf("[Warning][CH9329] buffer is full but no packet was successfully detected, dropping it: ");
        debug_print_buffer(ch9329->buffer, ch9329->buffer_count);
        ch9329->buffer_count = 0;
    }

    ch9329->buffer[ch9329->buffer_count++] = c;

    ch9329_decode_result_t result;
    ch9329_frame_t *frame = NULL;
retry:
    result = ch9329_try_decode_frame(ch9329, &frame);
    switch (result) {
        case CH9329L_DECODE_SUCCESS:
            printf("[Debug][CH9329] Packet received\n");
            ch9329->buffer_count = 0;
            if (!ch9329_enqueue_frame(ch9329, frame)) {
                printf("[Warning][CH9329] CH9329 frame queue is full, dropping frame");
                ch9329_frame_free(ch9329, frame);
            }
            return;
        case CH9329L_DECODE_NOT_ENOUGH_DATA:
            // printf("[Debug][CH9329] Not enough data\n");
            break;
        case CH9329L_DECODE_MALFORMED_DATA:
            printf("[Warning][CH9329] Malformed data when decoding frame. Dropping first byte:\n");
            debug_print_buffer(ch9329->buffer, ch9329->buffer_count);
            ch9329_drop_first_byte(ch9329);
            goto retry;
            break;
        case CH9329L_DECODE_CHECKSUM_MISMATCH:
            printf("[Warning][CH9329] Malformed data when decoding frame. Invalid checksum:\n");
            debug_print_buffer(ch9329->buffer, ch9329->buffer_count);
            ch9329->buffer_count = 0;
            break;
        case CH9329L_DECODE_NO_PACKET_AVAILABLE:
            printf("[Warning][CH9329] No packet available, packet are not consumed fast enough, dropping this:\n");
            debug_print_buffer(ch9329->buffer, ch9329->buffer_count);
            ch9329->buffer_count = 0;
            break;
        default:
            break;
    }
}

static uint8_t frame_header[] = {0x57, 0xAB};

static int send_command_packet(
    const ch9329_t *ch9329,
    const uint8_t addr,
    const uint8_t command,
    const uint8_t *data,
    const size_t data_len
) {
    uint8_t buffer[64];

    buffer[0] = frame_header[0];
    buffer[1] = frame_header[1];
    buffer[2] = addr;
    buffer[3] = command;
    buffer[4] = data_len;
    if (data_len + 5 + 1 >= sizeof(buffer)) {
        printf("[CH9229][Error] Data length exceeds buffer size\n");
        return 0;
    }
    if (data_len > 0)
        memcpy(buffer + 5, data, data_len);
    uint8_t checksum = 0;
    for (int i = 0; i < data_len + 5; i++) {
        checksum += buffer[i];
    }
    buffer[5 + data_len] = checksum;

    ch9329->send_data_func(ch9329, buffer, data_len + 6);
    return 1;
}

int ch9329_send_get_info(
    const ch9329_t *ch9329
) {
    return send_command_packet(ch9329, 0x00, CH9329_COMMAND_GET_INFO, NULL, 0);
}

int ch9329_send_get_para_cfg(
    const ch9329_t *ch9329
) {
    return send_command_packet(ch9329, 0x00, CH9329_COMMAND_GET_PARA_CFG, NULL, 0);
}

int ch9329_send_set_para_cfg(
    const ch9329_t *ch9329
) {
    return send_command_packet(ch9329, 0x00, CH9329_COMMAND_SET_PARA_CFG, (void*)&ch9329->para_cfg, sizeof(ch9329->para_cfg));
}


int ch9329_send_reset(
    const ch9329_t *ch9329
) {
    return send_command_packet(ch9329, 0x00, CH9329_COMMAND_RESET, NULL, 0);
}

int ch9329_send_get_usb_string(
    const ch9329_t *ch9329,
    const ch9329_usb_string_descriptor_t descriptor
) {
    const uint8_t data[1] = { descriptor };
    return send_command_packet(ch9329, 0x00, CH9329_COMMAND_GET_USB_STRING, data, 1);
}

static const char *ch9329_debug_usb_string_descriptor_to_string(
    ch9329_usb_string_descriptor_t descriptor
) {
    switch (descriptor) {
        case CH9329_USB_STRING_DESCRIPTOR_MANUFACTURER: return "MANUFACTURER";
        case CH9329_USB_STRING_DESCRIPTOR_PRODUCT: return "PRODUCT";
        case CH9329_USB_STRING_DESCRIPTOR_SERIAL_NUMBER: return "SERIAL_NUMBER";
        default: return "unk";
    }
}

const char *ch9329_debug_command_to_string(
    ch9329_command_t command
) {
    switch (command) {
        case CH9329_COMMAND_GET_INFO: return "GET_INFO";
        case CH9329_COMMAND_SEND_KB_GENERAL_DATA: return "SEND_KB_GENERAL_DATA";
        case CH9329_COMMAND_SEND_KB_MEDIA_DATA: return "SEND_KB_MEDIA_DATA";
        case CH9329_COMMAND_SEND_MS_ABS_DATA: return "SEND_MS_ABS_DATA";
        case CH9329_COMMAND_SEND_MS_REL_DATA: return "SEND_MS_REL_DATA";
        case CH9329_COMMAND_SEND_MY_HID_DATA: return "SEND_MY_HID_DATA";
        case CH9329_COMMAND_READ_MY_HID_DATA: return "READ_MY_HID_DATA";
        case CH9329_COMMAND_GET_PARA_CFG: return "GET_PARA_CFG";
        case CH9329_COMMAND_SET_PARA_CFG: return "SET_PARA_CFG";
        case CH9329_COMMAND_GET_USB_STRING: return "GET_USB_STRING";
        case CH9329_COMMAND_SET_USB_STRING: return "SET_USB_STRING";
        case CH9329_COMMAND_SET_DEFAULT_CFG: return "SET_DEFAULT_CFG";
        case CH9329_COMMAND_RESET: return "RESET";
        default: return "unk";
    }
}

void ch9329_debug_print_frame(
    ch9329_frame_t *frame
) {
    printf("[Debug][CH9329] ");
    if ((frame->command & 0xc0) == 0xc0) {
        printf("Error ");
    }
    if ((frame->command & 0x80) == 0x80) {
        printf("Response ");
    }
    printf("Frame: %s (0x%02x)\n", ch9329_debug_command_to_string(frame->command & 0xf), frame->command);
    switch (frame->command) {
        case CH9329_COMMAND_GET_USB_STRING | 0x80: {
            const ch9329_usb_string_frame_data_t *usb_string = (void *) frame->data;
            if (usb_string->len > sizeof(usb_string->data)) {
                printf("  usb_string: <INVALID_LEN>\n");
            } else {
                printf("  descriptor: %s (%x)\n", ch9329_debug_usb_string_descriptor_to_string(usb_string->descriptor), usb_string->descriptor);
                printf("  usb_string: $%.*s\n", usb_string->len, usb_string->data);
            }
            break;
        }
        case CH9329_COMMAND_GET_PARA_CFG | 0x80: {
            const ch9329_para_cfg_t *para_cfg = (void*)frame->data;
            printf("  chip_working_mode: 0x%02x\n", para_cfg->chip_working_mode);
            printf("  serial_port_communication_mode: 0x%02x\n", para_cfg->serial_port_communication_mode);
            printf("  serial_port_communication_address: 0x%02x\n", para_cfg->serial_port_communication_address);
            printf("  serial_baud_rate: %lu\n", (uint32_t)__bswap32(para_cfg->serial_port_communication_baud_rate));
            printf("  serial_port_communication_interval: %u\n", para_cfg->serial_port_communication_interval);
            printf("  usb_vid_pid: 0x%08lx\n", para_cfg->usb_vid_pid);
            printf("  usb_keyboard_upload_time_interval: %u\n", para_cfg->usb_keyboard_upload_time_interval);
            printf("  usb_keyboard_release_delay_time: %u\n", para_cfg->usb_keyboard_release_delay_time);
            printf("  usb_keyboard_auto_carriage_return_flag: %u\n", para_cfg->usb_keyboard_auto_carriage_return_flag);
            printf("  usb_keyboard_cr1: 0x%08lx\n", para_cfg->usb_keyboard_cr1);
            printf("  usb_keyboard_cr2: 0x%08lx\n", para_cfg->usb_keyboard_cr2);
            printf("  usb_keyboard_filter_start_char: 0x%08lx\n", para_cfg->usb_keyboard_filter_start_char);
            printf("  usb_keyboard_filter_end_char: 0x%08lx\n", para_cfg->usb_keyboard_filter_end_char);
            printf("  usb_keyboard_string_enable_flag: %u\n", para_cfg->usb_keyboard_string_enable_flag);
            printf("  usb_keyboard_fast_upload_flag: %u\n", para_cfg->usb_keyboard_fast_upload_flag);
            break;
        }
        default: {
            printf("  Data Len: %d\n", frame->data_length);
            printf("  Data: ");
            for (int i = 0; i < frame->data_length; i++) {
                printf("%02x ", frame->data[i]);
            }
            printf("\n");
            break;
        }
    }
}


int ch9329_send_keyboard_data(
    const ch9329_t *ch9329,
    const uint8_t *data,
    const size_t data_len
) {
    return send_command_packet(ch9329, 0x00, CH9329_COMMAND_SEND_KB_GENERAL_DATA, data, data_len);
}

int ch9329_send_mouse_rel_data(
    const ch9329_t *ch9329,
    const uint8_t *data,
    const size_t data_len
) {
    return send_command_packet(ch9329, 0x00, CH9329_COMMAND_SEND_MS_REL_DATA, data, data_len);
}

int ch9329_send_my_hid_data(
    const ch9329_t *ch9329,
    const uint8_t *data,
    const size_t data_len
) {
    return send_command_packet(ch9329, 0x00, CH9329_COMMAND_SEND_MY_HID_DATA, data, data_len);
}

static ch9329_t *ch9329_uart0 = NULL;
static ch9329_t *ch9329_uart1 = NULL;

static void ch9329_on_uart1_rx() {
    while (uart_is_readable(uart1)) {
        ch9329_received_byte(ch9329_uart1, uart_getc(uart1));
    }
}

static void ch9329_on_uart0_rx() {
    while (uart_is_readable(uart0)) {
        ch9329_received_byte(ch9329_uart0, uart_getc(uart0));
    }
}

static void ch9329_send_data_with_builtin_uart(
    const ch9329_t *ch9329,
    const uint8_t *data,
    const size_t len
) {
    uart_write_blocking(ch9329->user_data, data, len);
}

static void ch9329_change_baud_rate_with_builtin_uart(
    const ch9329_t *ch9329,
    const uint32_t baud_rate
) {
    uart_set_baudrate(ch9329->user_data, baud_rate);
}

ch9329_t *ch9329_init_with_builtin_uart(
    uart_inst_t *uart,
    const uint gpio_tx,
    const uint gpio_rx,
    const uint baud_rate
) {
    if (uart == uart0) {
        if (ch9329_uart0 != NULL) {
            printf("[CH9229][Error] UART0 already initialized\n");
            return NULL;
        }
    } else if (uart == uart1) {
        if (ch9329_uart1 != NULL) {
            printf("[CH9229][Error] UART1 already initialized\n");
            return NULL;
        }
    } else {
        printf("[CH9229][Error] Invalid uart\n");
        return NULL;
    }

    uart_init(uart, baud_rate);
    gpio_set_function(gpio_tx, GPIO_FUNC_UART);
    gpio_set_function(gpio_rx, GPIO_FUNC_UART);

    ch9329_t *ch9329 = malloc(sizeof(ch9329_t));
    memset(ch9329, 0, sizeof(ch9329_t));
    ch9329->baud_rate = baud_rate;
    ch9329->user_data = uart;
    ch9329->send_data_func = ch9329_send_data_with_builtin_uart;
    ch9329->change_baud_rate_func = ch9329_change_baud_rate_with_builtin_uart;

    if (uart == uart0) {
        irq_set_exclusive_handler(UART0_IRQ, ch9329_on_uart0_rx);
        irq_set_enabled(UART0_IRQ, true);
        uart_set_irqs_enabled(uart, true, false);
        ch9329_uart0 = ch9329;
    } else if (uart == uart1) {
        irq_set_exclusive_handler(UART1_IRQ, ch9329_on_uart1_rx);
        irq_set_enabled(UART1_IRQ, true);
        uart_set_irqs_enabled(uart, true, false);
        ch9329_uart1 = ch9329;
    }

    return ch9329;
}
