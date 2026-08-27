#include "computer_manager.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "node_link.h"

typedef struct {
    computer_t computers[MAX_COMPUTER];
} computer_manager_t;

static computer_manager_t computer_manager = {};

void computer_manager_init() {
    memset(&computer_manager, 0, sizeof(computer_manager));
    for (int i = 0; i < MAX_COMPUTER; i++) {
        computer_t *computer = &computer_manager.computers[i];
        computer->computer_id = i;
        computer->state = i == 0 ? COMPUTER_STATE_READY : COMPUTER_STATE_NOT_CONNECTED;
        if (i > 0) {
            queue_init(&computer->message_queue, sizeof(node_link_msg_t), 10);
        }
    }
    // Computer 0 is the local one
    computer_manager_configure_computer(LOCAL_COMPUTER_ID, -1, -1, -1);
}

void computer_manager_init_computer(
    const uint8_t computer_id
) {
    assert(computer_id < MAX_COMPUTER);
    computer_t *computer = &computer_manager.computers[computer_id];
    computer->state = COMPUTER_STATE_READY;
    memset(computer->hid_protocol_per_interface, 0, sizeof(computer->hid_protocol_per_interface));
    for (uint8_t kvm_hid_idx = 0; kvm_hid_idx < CFG_TUH_HID; kvm_hid_idx++) {
        computer_hid_report_t *itr = computer->hid_reports_per_interface[kvm_hid_idx];
        while (itr != nullptr) {
            computer_hid_report_t *prev = itr;
            itr = itr->next;
            free(prev->report_data);
            free(prev);
        }
        computer->hid_reports_per_interface[kvm_hid_idx] = nullptr;
    }
}

void computer_manager_configure_computer(
    const uint8_t computer_id,
    const uint8_t spi_selector_gpio,
    const uint8_t spi_ready_gpio,
    const uint8_t data_available_gpio
) {
    assert(computer_id < MAX_COMPUTER);
    computer_t *computer = &computer_manager.computers[computer_id];
    computer->spi_selector_gpio = spi_selector_gpio;
    computer->spi_ready_gpio = spi_ready_gpio;
    computer->data_available_gpio = data_available_gpio;
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
            saved_report->report_data_len = report_data_len;
            uint8_t *new_report_data = malloc(report_data_len);
            if (new_report_data == nullptr) {
                logf_critical("Failed to allocate memory for new_report_data size: %u", report_data_len);
                return false;
            }
            free(saved_report->report_data);
            saved_report->report_data = new_report_data;
            memcpy(saved_report->report_data, report_data, report_data_len);
        } else {
            saved_report->report_data_len = report_data_len;
            memcpy(saved_report->report_data, report_data, report_data_len);
        }
    }

    return true;
}

computer_t *computer_manager_get_computer(
    const uint8_t computer_id
) {
    assert(computer_id < MAX_COMPUTER);
    return &computer_manager.computers[computer_id];
}
