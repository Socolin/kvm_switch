#include "kvm_switch_config.h"

#include <stdint.h>
#include <string.h>

#include "config_persistence.h"
#include "logger.h"

#include "key_codes.h"

// https://pid.codes/pids/
// FIXME: Request PID when needed.
#define DEFAULT_USB_VID   0x1209
#define DEFAULT_USB_PID   0x50C0

#define SAVED_CONFIG_MAGIC 0x123456789ABCDEF0
#define CONFIG_VERSION 1 // Can be used in the future to handle config migration

typedef struct {
    uint64_t saved_config_magic;
    uint8_t version;
    keyboard_shortcut_t keyboard_shortcut[MAX_KEYBOARD_SHORTCUT];
    general_config_t general_config;
} kvm_config_t;

static kvm_config_t kvm_config = {};

static void kvm_config_init_from_default() {
    memset(&kvm_config, 0, sizeof(kvm_config_t));

    kvm_config.saved_config_magic = SAVED_CONFIG_MAGIC;
    kvm_config.version = CONFIG_VERSION;

    kvm_config.general_config.vid = DEFAULT_USB_VID;
    kvm_config.general_config.pid = DEFAULT_USB_PID;

    for (int i = 0; i < MAX_KEYBOARD_SHORTCUT; i++) {
        kvm_config.keyboard_shortcut[i].shortcut_id = i;
        kvm_config.keyboard_shortcut[i].enabled = false;
    }
    constexpr uint8_t shortcut_1_keys[] = {HID_KEYBOARD_USAGE_SCROLL_LOCK, HID_KEYBOARD_USAGE_1};
    constexpr shortcut_action_set_active_computer_data_t shortcut_1_data = {.computer_id = 0};
    kvm_config_set_shortcut(0, true, CHANGE_ACTIVE_COMPUTER_SET, 2, shortcut_1_keys, sizeof(shortcut_1_data), &shortcut_1_data);
    constexpr uint8_t shortcut_2_keys[] = {HID_KEYBOARD_USAGE_SCROLL_LOCK, HID_KEYBOARD_USAGE_2};
    constexpr shortcut_action_set_active_computer_data_t shortcut_2_data = {.computer_id = 1};
    kvm_config_set_shortcut(1, true, CHANGE_ACTIVE_COMPUTER_SET, 2, shortcut_2_keys, sizeof(shortcut_2_data), &shortcut_2_data);
}

void kvm_config_init() {
    config_persistence_read(&kvm_config, sizeof(kvm_config_t));
    if (kvm_config.saved_config_magic != SAVED_CONFIG_MAGIC) {
        logf_info("No configuration found, using default values");
        kvm_config_init_from_default();
    } else {
        log_info("Configuration found, using saved values");
        if (kvm_config.version != CONFIG_VERSION) {
            // In the future we can have some migration logic here
            logf_warning("Invalid configuration version: %d, using default values", kvm_config.version);
            kvm_config_init_from_default();
        }
    }
}

void kvm_config_save() {
    config_persistence_save(&kvm_config, sizeof(kvm_config_t));
}

general_config_t *kvm_config_get_general() {
    return &kvm_config.general_config;
}

keyboard_shortcut_t *kvm_config_get_shortcut(
    const uint8_t shortcut_id
) {
    if (shortcut_id >= MAX_KEYBOARD_SHORTCUT) {
        logf_error("Invalid shortcut ID: %d", shortcut_id);
        return nullptr;
    }
    return &kvm_config.keyboard_shortcut[shortcut_id];
}

void kvm_config_set_shortcut(
    const uint8_t shortcut_id,
    const bool enabled,
    const shortcut_action_t action,
    const uint8_t key_count,
    const uint8_t *keys,
    const uint8_t data_len,
    const void *data
) {
    if (shortcut_id >= MAX_KEYBOARD_SHORTCUT) {
        logf_error("Invalid shortcut ID: %d", shortcut_id);
        return;
    }
    if (key_count > MAX_KEYS_PER_SHORTCUT) {
        logf_error("Invalid key count: %d", key_count);
        return;
    }
    if (data_len > MAX_DATA_PER_SHORTCUT) {
        logf_error("Invalid data length: %d", data_len);
        return;
    }

    keyboard_shortcut_t *shortcut = &kvm_config.keyboard_shortcut[shortcut_id];
    shortcut->key_count = key_count;
    memcpy(shortcut->keys, keys, key_count);
    shortcut->enabled = enabled;
    shortcut->action = action;
    shortcut->data_len = data_len;
    memcpy(shortcut->data, data, data_len);
}

const keyboard_shortcut_t *kvm_config_first_matching_shortcut(
    const uint8_t key_count,
    const uint8_t *keys
) {
    for (uint8_t shortcut_id = 0; shortcut_id < MAX_KEYBOARD_SHORTCUT; shortcut_id++) {
        const keyboard_shortcut_t *shortcut = &kvm_config.keyboard_shortcut[shortcut_id];
        if (!shortcut->enabled || shortcut->key_count != key_count)
            continue;

        bool match = true;
        for (uint8_t key_idx = 0; key_idx < key_count; key_idx++) {
            bool key_found = false;
            for (uint8_t key_idx2 = 0; key_idx2 < key_count; key_idx2++) {
                if (shortcut->keys[key_idx] == keys[key_idx2]) {
                    key_found = true;
                    break;
                }
            }
            if (!key_found) {
                match = false;
                break;
            }
        }
        if (match)
            return shortcut;
    }

    return nullptr;
}
