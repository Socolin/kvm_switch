#include "kvm_switch_config.h"

#include <stdint.h>
#include <string.h>

#include "logger.h"

typedef struct {
    keyboard_shortcut_t keyboard_shortcut[MAX_KEYBOARD_SHORTCUT];
} kvm_config_t;

static kvm_config_t kvm_config = {};

void kvm_config_init() {
    memset(&kvm_config, 0, sizeof(kvm_config_t));
    for (int i = 0; i < MAX_KEYBOARD_SHORTCUT; i++) {
        kvm_config.keyboard_shortcut[i].shortcut_id = i;
        kvm_config.keyboard_shortcut[i].enabled = false;
    }
}

void kvm_config_set_shortcut(
    const uint8_t shortcut_id,
    const uint8_t key_count,
    const uint8_t *keys,
    const uint8_t data_len,
    const uint8_t *data
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
    shortcut->data_len = data_len;
    memcpy(shortcut->data, data, data_len);
}

const keyboard_shortcut_t *kvm_config_first_matching_shortcut(
    const uint8_t key_count,
    const uint8_t *keys
) {
    for (uint8_t shortcut_id = 0; shortcut_id < MAX_KEYBOARD_SHORTCUT; shortcut_id++) {
        keyboard_shortcut_t *shortcut = &kvm_config.keyboard_shortcut[shortcut_id];
        if (!shortcut->enabled || shortcut->key_count != key_count)
            continue;
        bool match = true;
        for (uint8_t key_idx = 0; key_idx < key_count; key_idx++) {
            bool key_found = false;
            for (uint8_t key_idx2 = 0; key_idx2 < key_count; key_idx2++) {
                if (shortcut->keys[key_idx] != keys[key_idx2]) {
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
