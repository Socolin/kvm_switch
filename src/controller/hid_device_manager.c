#include "hid_device_manager.h"

#include <string.h>

#include "logger.h"

typedef struct {
    hid_device_t devices[MAX_HID_DEVICE];
} hid_device_manager_t;

static hid_device_manager_t hid_device_manager;

void hid_device_manager_init() {
    memset(&hid_device_manager, 0, sizeof(hid_device_manager));
    for (int i = 0; i < MAX_HID_DEVICE; i++) {
        hid_device_manager.devices[i].dev_addr = i + 1;
    }
}

static bool hid_device_manager_validate_dev_addr(
    const uint8_t dev_addr
) {
    if (dev_addr == 0) {
        return false;
    }
    if (dev_addr > MAX_HID_DEVICE) {
        logf_critical("Invalid device address: %u", dev_addr);
        return false;
    }
    return true;
}

const hid_device_t *hid_device_manager_get(
    const uint8_t dev_addr
) {
    if (!hid_device_manager_validate_dev_addr(dev_addr)) {
        return nullptr;
    }
    return &hid_device_manager.devices[dev_addr - 1];
}

void hid_device_manager_mount_device(
    const uint8_t dev_addr
) {
    if (!hid_device_manager_validate_dev_addr(dev_addr)) {
        return;
    }
    hid_device_manager.devices[dev_addr - 1].is_mounted = true;
}

void hid_device_manager_unmount_device(
    const uint8_t dev_addr
) {
    if (!hid_device_manager_validate_dev_addr(dev_addr)) {
        return;
    }
    hid_device_manager.devices[dev_addr - 1].is_mounted = false;
    hid_device_manager.devices[dev_addr - 1].manufacturer_name_len = 0;
    hid_device_manager.devices[dev_addr - 1].product_name_len = 0;
}

void hid_device_manager_set_manufacturer_name(
    const uint8_t dev_addr,
    const uint16_t *string,
    const uint8_t string_len
) {
    if (!hid_device_manager_validate_dev_addr(dev_addr)) {
        return;
    }
    memcpy(hid_device_manager.devices[dev_addr - 1].manufacturer_name, string, string_len);
    hid_device_manager.devices[dev_addr - 1].manufacturer_name_len = string_len;
}

void hid_device_manager_set_product_name(
    const uint8_t dev_addr,
    const uint16_t *string,
    const uint8_t string_len
) {
    if (!hid_device_manager_validate_dev_addr(dev_addr)) {
        return;
    }
    memcpy(hid_device_manager.devices[dev_addr - 1].product_name, string, string_len);
    hid_device_manager.devices[dev_addr - 1].product_name_len = string_len;
}
