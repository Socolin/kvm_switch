#pragma once
#include <stdint.h>

#include "tusb_config.h"
#include "pico/util/queue.h"

#define MAX_COMPUTER 2

/**
 * Store the report sent by the computer for a given hid interface, like the LEDs status.
 * So when we switch to this computer, we can send the report to the hid.
 */
typedef struct computer_hid_report {
    uint8_t report_id;
    uint8_t report_type; /**< \see hid_report_type_t */
    uint16_t report_data_len;
    uint8_t *report_data;
    struct computer_hid_report *next;
} computer_hid_report_t;

typedef enum {
    COMPUTER_STATE_NOT_CONNECTED,
    COMPUTER_STATE_READY,
} computer_state_t;

typedef struct {
    uint8_t computer_id;
    uint8_t spi_selector_gpio;
    uint8_t spi_ready_gpio;
    uint8_t data_available_gpio;
    computer_state_t state;
    uint8_t hid_protocol_per_interface[CFG_TUH_HID]; // BOOT / REPORT
    computer_hid_report_t *hid_reports_per_interface[CFG_TUH_HID];
    queue_t message_queue;
} computer_t;

void computer_manager_init();

void computer_manager_configure_computer(
    uint8_t computer_id,
    uint8_t spi_selector_gpio,
    uint8_t spi_ready_gpio,
    uint8_t data_available_gpio
);

void computer_manager_init_computer(
    uint8_t computer_id
);

void computer_manager_set_hid_protocol(
    uint8_t computer_id,
    uint8_t kvm_hid_idx,
    uint8_t hid_protocol
);

bool computer_manager_set_report(
    uint8_t computer_id,
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    uint8_t report_type,
    uint8_t const *report_data,
    uint16_t report_data_len
);

computer_t *computer_manager_get_computer(
    uint8_t computer_id
);
