#pragma once

#define MAX_HID_DEVICE 2
#include <stdint.h>

typedef struct {
    bool is_mounted;
    uint8_t dev_addr;
    uint8_t manufacturer_name_len;
    uint16_t manufacturer_name[128];
    uint8_t product_name_len;
    uint16_t product_name[128];
} hid_device_t;

void hid_device_manager_init();

const hid_device_t *hid_device_manager_get(
    uint8_t dev_addr
);

void hid_device_manager_mount_device(
    uint8_t dev_addr
);

void hid_device_manager_unmount_device(
    uint8_t dev_addr
);

void hid_device_manager_set_manufacturer_name(
    uint8_t dev_addr,
    const uint16_t *string,
    uint8_t string_len
);

void hid_device_manager_set_product_name(
    uint8_t dev_addr,
    const uint16_t *string,
    uint8_t string_len
);
