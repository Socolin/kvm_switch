#pragma once

#include <stdint.h>

#define MAX_KEYBOARD_SHORTCUT 10
#define MAX_KEYS_PER_SHORTCUT 6
#define MAX_DATA_PER_SHORTCUT 16

typedef enum {
    CHANGE_ACTIVE_COMPUTER_NEXT,
    CHANGE_ACTIVE_COMPUTER_PREVIOUS,
    CHANGE_ACTIVE_COMPUTER_SET,
    CHANGE_DEVICE_ACTIVE_COMPUTER_SET,
} shortcut_action_t;

typedef struct {
    uint8_t computer_id;
} shortcut_action_set_active_computer_data_t;

typedef struct {
    uint8_t computer_id;
    uint8_t dev_addr;
} shortcut_action_set_device_active_computer_data_t;

typedef struct __attribute__((packed)) {
    uint8_t shortcut_id;
    bool enabled;
    shortcut_action_t action;
    uint8_t key_count;
    uint8_t keys[MAX_KEYS_PER_SHORTCUT];
    uint8_t data_len;
    uint8_t data[MAX_DATA_PER_SHORTCUT];
} keyboard_shortcut_t;

typedef struct {
    uint16_t vid;
    uint16_t pid;
} general_config_t;

void kvm_config_init();

void kvm_config_save();

general_config_t *kvm_config_get_general(
);

keyboard_shortcut_t *kvm_config_get_shortcut(
    uint8_t shortcut_id
);

void kvm_config_set_shortcut(
    uint8_t shortcut_id,
    bool enabled,
    shortcut_action_t action,
    uint8_t key_count,
    const uint8_t *keys,
    uint8_t data_len,
    const void *data
);

const keyboard_shortcut_t *kvm_config_first_matching_shortcut(
    uint8_t key_count,
    const uint8_t *keys
);
