#ifndef HID_H
#define HID_H
#include <stdint.h>

// ╔══════════════════════════════════╗
// ║     Hid to computer messages     ║
// ╚══════════════════════════════════╝

typedef enum {
    HID_TO_COMPUTER_KEYBOARD_MESSAGE,
    HID_TO_COMPUTER_MOUSE_MESSAGE,
    HID_TO_COMPUTER_MEDIA_MESSAGE,
} hid_to_computer_message_opcode_t;

typedef struct {
    hid_to_computer_message_opcode_t opcode;
    size_t data_len;
    uint8_t data[128];
} hid_to_computer_message_t;

typedef struct {
    uint8_t buttons;
    int16_t x;
    int16_t y;
    int8_t wheel;
    int8_t pan;
} htc_message_mouse_data_t;

// ╔══════════════════════════════════╗
// ║     Computer to HID messages     ║
// ╚══════════════════════════════════╝

typedef enum {
    COMPUTER_TO_HID_UPDATE_KEYBOARD_LEDS,
} computer_to_hid_message_opcode_t;

typedef struct {
    computer_to_hid_message_opcode_t opcode;
    uint8_t data[128];
} computer_to_hid_message_t;

#endif
