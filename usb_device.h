#pragma once

#include <stdint.h>

void usb_device_init(
    uint8_t computer_id,
    uint16_t vid,
    uint16_t pid
);

void usb_device_task();

void usb_device_connect_to_computer();
