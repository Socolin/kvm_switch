#pragma once

typedef enum {
    HID_USAGE_PAGE_GENERIC_DESKTOP_PAGE = 0x01,
    HID_USAGE_PAGE_KEYBOARD = 0x07,
} hid_usage_page_t;

typedef enum {
    HID_USAGE_GENERIC_DESKTOP_MOUSE = 0x02,
    HID_USAGE_GENERIC_DESKTOP_KEYBOARD = 0x06,
    HID_USAGE_GENERIC_DESKTOP_KEYPAD = 0x07,
} hid_generic_desktop_usage_t;
