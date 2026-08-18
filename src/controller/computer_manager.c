#include "computer_manager.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../shared/logger.h"

typedef struct {
    computer_t computers[MAX_COMPUTER];
} computer_manager_t;

static computer_manager_t computer_manager = {};

void computer_manager_init() {
    memset(&computer_manager, 0, sizeof(computer_manager));
    for (int i = 0; i < MAX_COMPUTER; i++) {
        computer_manager.computers[i].computer_id = i;
    }
    // Computer 0 is the local one
    computer_manager_configure_computer(0, -1, -1);
}

void computer_manager_configure_computer(
    const uint8_t computer_id,
    const uint8_t spi_selector_gpio_pin,
    const uint8_t data_available_gpio_pin
) {
    assert(computer_id < MAX_COMPUTER);
    computer_t *computer = &computer_manager.computers[computer_id];
    computer->spi_selector_gpio_pin = spi_selector_gpio_pin;
    computer->data_available_gpio_pin = data_available_gpio_pin;
}

void computer_manager_set_hid_protocol(
    const uint8_t computer_id,
    const uint8_t kvm_hid_idx,
    const uint8_t hid_protocol
) {
    assert(computer_id < MAX_COMPUTER);
    computer_t *computer = &computer_manager.computers[computer_id];
    computer->hid_protocol_per_interface[kvm_hid_idx] = hid_protocol;
}

bool computer_manager_set_report(
    const uint8_t computer_id,
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint8_t report_type,
    uint8_t const *report_data,
    const uint16_t report_data_len
) {
    assert(computer_id < MAX_COMPUTER);
    computer_t *computer = &computer_manager.computers[computer_id];
    computer_hid_report_t *saved_report = nullptr;

    computer_hid_report_t *itr = computer->hid_reports_per_interface[kvm_hid_idx];
    while (itr != nullptr) {
        // FIXME: Should we check report_type?
        if (itr->report_id == report_id) {
            saved_report = itr;
            break;
        }
        itr = itr->next;
    }

    if (saved_report == nullptr) {
        saved_report = calloc(sizeof(computer_hid_report_t), 1);
        if (saved_report == nullptr) {
            log_critical("Failed to allocate memory for computer_hid_report_t");
            return false;
        }
        saved_report->report_id = report_id;
        saved_report->report_type = report_type;
        saved_report->report_data_len = report_data_len;
        saved_report->report_data = malloc(report_data_len);
        if (saved_report->report_data == nullptr) {
            logf_critical("Failed to allocate memory for saved_report->report_data size: %u", report_data_len);
            free(saved_report);
            return false;
        }
        memcpy(saved_report->report_data, report_data, report_data_len);
        saved_report->next = computer->hid_reports_per_interface[kvm_hid_idx];
        computer->hid_reports_per_interface[kvm_hid_idx] = saved_report;
    } else {
        if (saved_report->report_data_len < report_data_len) {
            free(saved_report->report_data);
            saved_report->report_data_len = report_data_len;
            saved_report->report_data = malloc(report_data_len);
            if (saved_report->report_data == nullptr) {
                logf_critical("Failed to allocate memory for saved_report->report_data size: %u", report_data_len);
                free(saved_report);
                return false;
            }
            memcpy(saved_report->report_data, report_data, report_data_len);
        } else {
            saved_report->report_data_len = report_data_len;
            memcpy(saved_report->report_data, report_data, report_data_len);
        }
    }

    return true;
}
