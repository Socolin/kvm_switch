#ifndef CH9329_H
#define CH9329_H

#include "pico/stdlib.h"

// ╔══════════════════════════════════╗
// ║             Frames               ║
// ╚══════════════════════════════════╝

/*
 * ┌─────────┬─────────┬─────────┬─────────┬────────────────┬─────────┐
 * │ HEAD    │ ADDR    │ CMD     │ LEN     │ DATA           │ SUM     │
 * ├─────────┼─────────┼─────────┼─────────┼────────────────┼─────────┤
 * │ 2 bytes │ 1 byte  │ 1 byte  │ 1 byte  │ N bytes (0-64) │ 1 byte  │
 * └─────────┴─────────┴─────────┴─────────┴────────────────┴─────────┘
 */

#define CH9329_MAX_PACKET_DATA_LEN 64
#define CH9329_MAX_PACKET_LEN (CH9329_MAX_PACKET_DATA_LEN + 6)

typedef enum {
    // Command sent to the CH9329 are 0x00-0x0F
    // Response command are the same with | 0x80
    // Error response are the same with | 0xC0
    CH9329_COMMAND_GET_INFO = 0x1,
    CH9329_COMMAND_SEND_KB_GENERAL_DATA = 0x2,
    CH9329_COMMAND_SEND_KB_MEDIA_DATA = 0x3,
    CH9329_COMMAND_SEND_MS_ABS_DATA = 0x4,
    CH9329_COMMAND_SEND_MS_REL_DATA = 0x5,
    CH9329_COMMAND_SEND_MY_HID_DATA = 0x6,
    CH9329_COMMAND_READ_MY_HID_DATA = 0x7, // Only sent by the CH9329, with 0x87
    CH9329_COMMAND_GET_PARA_CFG = 0x8,
    CH9329_COMMAND_SET_PARA_CFG = 0x9,
    CH9329_COMMAND_GET_USB_STRING = 0xA,
    CH9329_COMMAND_SET_USB_STRING = 0xB,
    CH9329_COMMAND_SET_DEFAULT_CFG = 0xC,
    CH9329_COMMAND_RESET = 0xF,
} ch9329_command_t;

typedef enum {
    DEF_CMD_SUCCESS = 0x00,
    DEF_CMD_ERR_TIMEOUT = 0xE1,
    DEF_CMD_ERR_HEAD = 0xE2,
    DEF_CMD_ERR_CMD = 0xE3,
    DEF_CMD_ERR_SUM = 0xE4,
    DEF_CMD_ERR_PARA = 0xE5,
    DEF_CMD_ERR_OPERATE = 0xE6,
} ch9329_error_code_t;

typedef struct {
    uint8_t address;
    uint8_t command;
    uint8_t data_length;
    uint8_t data[CH9329_MAX_PACKET_DATA_LEN];
    uint8_t checksum;
} ch9329_frame_t;


// ┌──────────────────────────────────┐
// │        CMD_GET_PARA_CFG          │
// └──────────────────────────────────┘

typedef struct {
    /**
     * Values: 0x00-0x03, 0x80---0x83, and the default is 0x80;
     * 0x00: Working mode 0 set by software, standard USB keyboard (common + multimedia) + USB mouse (absolute mouse + relative mouse);
     * 0x01: Working mode 1 set by software, standard USB keyboard (common);
     * 0x02: Working mode 2 set by software, standard USB mouse (absolute mouse + relative mouse);
     * 0x03: Working mode 3 set by software, standard USB custom HID device;
     * 0x80: Working mode 0 set by hardware pin, standard USB keyboard (common + multimedia) + USB mouse (absolute mouse standard + relative mouse); currently MODE1 pin is high level, MODE0 pin is high level
     * 0x81: Working mode 1 set by hardware pins, standard USB keyboard (common); current
     * 0x82: Working mode 2 set by hardware pins, standard USB mouse (absolute mouse + relative mouse); currently MODE1 pin is low level, MODE0 pin is high level;
     * 0x83: Working mode 3 set by hardware pin, standard USB custom HID device; current * MODE1 pin is low level, MODE0 pin is low level;
     */
    uint8_t chip_working_mode;
    /**
     * Values: 0x00-0x02, 0x80---0x82, the default is 0x80;
     * 0x00: Serial port communication mode 0 set by software, protocol transmission mode;
     * 0x01: Serial port communication mode 1 set by software, ASCII mode;
     * 0x02: Serial port communication mode 2 set by software, transparent transmission mode;
     * 0x80: Serial port communication mode 0 set by hardware pins, protocol transmission mode; current CFG1 pin is high level, CFG0 pin is high level;
     * 0x81: Serial port communication mode 1 set by hardware pins, ASCII mode; current CFG1 pin is high level, CFG0 pin is low level;
     * 0x82: Serial port communication mode 2 set by hardware pins, transparent transmission mode; the current CFG1 pin is low level, and the CFG0 pin is high level;
     */
    uint8_t serial_port_communication_mode;
    /**
     * Values: 0x00--0xFF, the default is 0x00;
     */
    uint8_t serial_port_communication_address;
    /**
    * High byte first, the default is 0x00002580, that is, the baud rate is 9600bps;
     */
    uint32_t serial_port_communication_baud_rate;
    uint16_t reserved;
    /**
     * Values: 0x0000--0xFFFF, the default is 3, and the unit is mS
     * that is, if the chip does not receive the next byte for more than 3mS, it means that the packet is over
     */
    uint16_t serial_port_communication_interval;
    /**
     * The VID and PID of the 4-byte chip USB, the default chip VID is 0x1A86, and the PID is 0xE129.
     * In different working modes, the PID is different
     */
    uint32_t usb_vid_pid;
    /**
    * USB keyboard upload time interval (only valid in ASCII mode), the effective
    * range is 0x0000--0xFFFF, the default is 0, the unit is mS, that is, the chip uploads the next
    * packet immediately after uploading the first packet of data packet data;
     */
    uint16_t usb_keyboard_upload_time_interval;
    /**
     * 2-byte chip USB keyboard release delay time (only valid in ASCII mode), the valid
     * range is 0x0000--0xFFFF, the default is 1, and the unit is mS, that is, 1 mS after the chip
     * uploads the button and presses the data packet Upload key release data packet;
     */
    uint16_t usb_keyboard_release_delay_time;
    /**
     * 1-byte chip USB keyboard automatic carriage return flag (only valid in ASCII mode),
     * the valid range is 0x00--0x01, 0x00 means no automatic carriage return, 0x01 means automatic
     * carriage return after the end of the package;
     */
    uint8_t usb_keyboard_auto_carriage_return_flag;
    /**
    * 8-byte chip USB keyboard carriage return (only valid in ASCII mode), 4 bytes in one
    * group, 2 groups in total, that is, 2 different carriage return characters can be set, and the
    * default ASCII value is 0x0D Carriage return;
    */
    uint32_t usb_keyboard_cr1;
    uint32_t usb_keyboard_cr2;
    /**
     * 8 bytes of chip USB keyboard filter start and end character strings, the first 4 bytes are filter start
     * characters, and the last 4 bytes are filter end characters;
     */
    uint32_t usb_keyboard_filter_start_char;
    uint32_t usb_keyboard_filter_end_char;
    /**
     * 1 byte chip USB string enable flag
     * Bit 7: 0 means disable; 1 means enable custom string descriptor;
     * Bits 6-3: Reserved;
     * Bit 2: 0 means disable; 1 means enable custom vendor string descriptor;
     * Bit 1: 0 means disable; 1 means enable custom product string descriptor;
     * Bit 0: 0 means disable; 1 means enable custom serial number string descriptor;
     */
    uint8_t usb_keyboard_string_enable_flag;
    /**
     * 1 byte chip USB keyboard fast upload flag (only valid in ASCII mode), the effective
     * range is 0x00--0x01, 0x00 means that the USB keyboard upload speed is normal, 0x01 means
     * enable the USB keyboard fast upload mode, enable fast After uploading mode, after uploading 1
     * character, the release button packet will not be sent, and the next character will be
     * uploaded, and the release button packet will be sent only after all characters are uploaded.
     */
    uint8_t usb_keyboard_fast_upload_flag;
    uint8_t reserved2[12];
} __attribute__((__packed__)) ch9329_para_cfg_t;


// ┌──────────────────────────────────┐
// │         XXX_USB_STRING           │
// └──────────────────────────────────┘

typedef enum {
    CH9329_USB_STRING_DESCRIPTOR_MANUFACTURER,
    CH9329_USB_STRING_DESCRIPTOR_PRODUCT,
    CH9329_USB_STRING_DESCRIPTOR_SERIAL_NUMBER,
} ch9329_usb_string_descriptor_t;

typedef struct {
    uint8_t descriptor; // ch9329_usb_string_descriptor_t
    uint8_t len;
    char data[23];
} __attribute__((__packed__)) ch9329_usb_string_frame_data_t;

// ╔══════════════════════════════════╗
// ║               Core               ║
// ╚══════════════════════════════════╝

#define CH9329_RECEIVING_BUFFER_MAX_LEN CH9329_MAX_PACKET_LEN
#define CH9329_MESSAGE_QUEUE_SIZE 32

typedef struct ch9329 ch9329_t;

typedef void (*ch9329_send_data_func_t)(
    const ch9329_t *ch9329,
    const uint8_t *data,
    size_t len
);

typedef void (*ch9329_change_baud_rate_func_t)(
    const ch9329_t *ch9329,
    uint32_t baud_rate
);

typedef struct ch9329 {
    uint32_t baud_rate;
    uint16_t buffer_count;
    uint8_t buffer[CH9329_RECEIVING_BUFFER_MAX_LEN];
    ch9329_frame_t *frames[CH9329_MESSAGE_QUEUE_SIZE];
    ch9329_frame_t frames_pool[CH9329_MESSAGE_QUEUE_SIZE];
    uint8_t frames_from_pool_in_use[CH9329_MESSAGE_QUEUE_SIZE];
    volatile uint8_t frames_head;
    volatile uint8_t frames_tail;
    void *user_data;
    ch9329_send_data_func_t send_data_func;
    ch9329_change_baud_rate_func_t change_baud_rate_func;
    ch9329_para_cfg_t para_cfg;
} ch9329_t;

/**
 * Initialize a new ch9329 device on a default uart of the pico pi, either uart0 or uart1
 * @param uart the default uart to use
 * @param gpio_tx The tx pin (Connected to the RX pin of the ch9329) Must be a valid pin for the given uart
 * @param gpio_rx The rx pin (Connected to the TX pin of the ch9329) Must be a valid pin for the given uart
 * @param baud_rate The baudrate to use, see BA0 and BA1 on the ch9329
 * @return a pointer to the initialized ch9329 device
 */
ch9329_t *ch9329_init_with_builtin_uart(
    uart_inst_t *uart,
    uint gpio_tx,
    uint gpio_rx,
    uint baud_rate
);

// ╔══════════════════════════════════╗
// ║           Functions              ║
// ╚══════════════════════════════════╝


int ch9329_send_get_info(
    const ch9329_t *ch9329
);

int ch9329_send_get_para_cfg(
    const ch9329_t *ch9329
);

int ch9329_send_set_para_cfg(
    const ch9329_t *ch9329
);

int ch9329_send_reset(
    const ch9329_t *ch9329
);

int ch9329_send_get_usb_string(
    const ch9329_t *ch9329,
    ch9329_usb_string_descriptor_t descriptor
);

int ch9329_send_keyboard_data(
    const ch9329_t *ch9329,
    const uint8_t *data,
    size_t data_len
);

int ch9329_send_mouse_rel_data(
    const ch9329_t *ch9329,
    const uint8_t *data,
    size_t data_len
);

int ch9329_send_my_hid_data(
    const ch9329_t *ch9329,
    const uint8_t *data,
    size_t data_len
);

ch9329_frame_t *ch9329_dequeue_frame(
    ch9329_t *ch9329
);

void ch9329_frame_free(
    ch9329_t *ch9329,
    ch9329_frame_t *frame
);

// ┌──────────────────────────────────┐
// │               Debug              │
// └──────────────────────────────────┘

void ch9329_debug_print_frame(
    ch9329_frame_t *frame
);

#endif
