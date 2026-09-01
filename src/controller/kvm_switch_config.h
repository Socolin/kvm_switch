#pragma once

#include <stdint.h>

#define MAX_KEYBOARD_SHORTCUT 10
#define MAX_KEYS_PER_SHORTCUT 5
#define MAX_DATA_PER_SHORTCUT 16

typedef enum {
    CHANGE_ACTIVE_COMPUTER_NEXT,
    CHANGE_ACTIVE_COMPUTER_PREVIOUS,
    CHANGE_ACTIVE_COMPUTER_SET,
} shortcut_action_t;

typedef struct __attribute__((packed)) {
    uint8_t shortcut_id;
    bool enabled;
    shortcut_action_t action;
    uint8_t key_count;
    uint8_t keys[MAX_KEYS_PER_SHORTCUT];
    uint8_t data_len;
    uint8_t data[MAX_DATA_PER_SHORTCUT];
} keyboard_shortcut_t;

void kvm_config_init();

void kvm_config_set_shortcut(
    uint8_t shortcut_id,
    shortcut_action_t action,
    uint8_t key_count,
    const uint8_t *keys,
    uint8_t data_len,
    const uint8_t *data
);

const keyboard_shortcut_t *kvm_config_first_matching_shortcut(
    uint8_t key_count,
    const uint8_t *keys
);
