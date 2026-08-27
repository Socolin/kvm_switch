#include "node_link_node.h"

#include <stdlib.h>
#include <string.h>

#include "kvm_switch_node.h"
#include "../shared/node_link.h"
#include "../shared/logger.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/util/queue.h"

typedef struct {
    node_link_t link;
    queue_t message_queue;
} node_link_node_t;

static node_link_node_t node = {};

#define SPI_READY_GPIO 6
#define DATA_READY_GPIO 7

static void node_link_node_process_received_message(const node_link_msg_t *message, void *udata);

void node_link_node_init() {
    gpio_init(SPI_READY_GPIO);
    gpio_set_dir(SPI_READY_GPIO, GPIO_OUT);
    gpio_put(SPI_READY_GPIO, 0);
    gpio_init(DATA_READY_GPIO);
    gpio_set_dir(DATA_READY_GPIO, GPIO_OUT);
    gpio_put(DATA_READY_GPIO, 0);

    queue_init(&node.message_queue, sizeof(node_link_msg_t), 16);
    node_link_init_node(&node.link, spi0, SPI_READY_GPIO, node_link_node_process_received_message);
}

static void node_link_node_signal_data_ready() {
    gpio_put(DATA_READY_GPIO, 1);
}

static void node_link_node_clear_data_ready() {
    gpio_put(DATA_READY_GPIO, 0);
}

static void node_link_node_process_received_message(
    const node_link_msg_t *message,
    [[maybe_unused]] void *udata
) {
    logf_debug("opcode: %u, data_len: %u", message->header.opcode, message->header.data_len);

    switch (message->header.opcode) {
        case NL_CTRL_MESSAGE_OP_INIT: {
            logf_info("Init message received from controller");
            break;
        }
        case NL_CTRL_MESSAGE_OP_HID_MOUNT: {
            auto const message_data = (nl_ctrl_msg_hid_mount_data_t *) message->data;
            uint8_t *report_desc = malloc(message->extra_data[0].data_len);
            if (report_desc == nullptr) {
                log_critical("failed to allocate memory for report descriptor");
                return;
            }
            memcpy(report_desc, message->extra_data[0].data, message->extra_data[0].data_len);

            kvm_switch_node_enqueue_hid_mount(
                message_data->dev_addr,
                message_data->host_hid_idx,
                message_data->kvm_hid_idx,
                message_data->itf_protocol,
                message_data->vid,
                message_data->pid,
                report_desc,
                message->extra_data[0].data_len
            );
            break;
        }
        case NL_CTRL_MESSAGE_OP_HID_UMOUNT: {
            auto const message_data = (nl_ctrl_msg_hid_umount_data_t *) message->data;
            kvm_switch_node_enqueue_hid_umount(message_data->dev_addr, message_data->host_hid_idx);
            break;
        }
        case NL_CTRL_MESSAGE_OP_HID_REPORT: {
            auto const message_data = (nl_ctrl_msg_hid_report_data_t *) message->data;
            kvm_switch_node_enqueue_hid_report(
                message_data->kvm_hid_idx,
                message_data->report_id,
                message_data->report_data_len,
                message_data->report_data
            );
            break;
        }
        case NL_CTRL_MESSAGE_OP_START_USB_DEVICE: {
            logf_info("Start USB message received from controller");
            auto const message_data = (nl_ctrl_msg_start_usb_device_data_t *) message->data;
            kvm_switch_node_enqueue_connect_usb_device(message_data->vid, message_data->pid);
            break;
        }
        default: {
            logf_error("invalid opcode: %04x", message->header.opcode);
            break;
        }
    }
}

void node_link_node_run() {
    const uint64_t delay_before_retry_after_error = node_link_get_us_delay_before_retry(&node.link);

    while (true) {
        node_link_msg_t message;
        if (!queue_try_peek(&node.message_queue, &message)) {
            // Nothing to send
            if (!node_link_send_message_blocking(&node.link, nullptr, nullptr)) {
                // When a transmission happen, wait before trying again so the host can timeout and will not
                // catch mid transmission data during next retry
                node_link_drain_rx(&node.link);
                log_warning("Transmission failed, waiting before retrying");
                sleep_us(delay_before_retry_after_error + 1'000);
            }
        } else {
            if (!node_link_send_message_blocking(&node.link, &message, nullptr)) {
                // When a transmission happen, wait before trying again so the host can timeout and will not
                // catch mid transmission data during next retry
                node_link_drain_rx(&node.link);
                log_warning("Transmission failed, waiting before retrying");
                sleep_us(delay_before_retry_after_error + 1'000);
            } else {
                queue_remove_blocking(&node.message_queue, nullptr);
                node_link_dispose_message(&message);
                if (queue_is_empty(&node.message_queue)) {
                    node_link_node_clear_data_ready();
                }
            }
        }
    }
}


// ╔══════════════════════════════════╗
// ║     Enqueue message to send      ║
// ╚══════════════════════════════════╝

static bool node_link_node_enqueue_message(
    const node_link_node_msg_opcode_t opcode,
    const void *data,
    const size_t data_len,
    const void *extra_data_1,
    const size_t extra_data_1_len,
    const bool should_free_once_sent
) {
    logf_debug("opcode: %u, data_len: %u", opcode, data_len);
    if (queue_is_full(&node.message_queue)) {
        logf_warning("Message queue is full");
        return false;
    }

    node_link_msg_t message = {
        .header = {
            .opcode = opcode,
            .data_len = data_len,
            .extra_data_count = extra_data_1 ? 1 : 0,
        },
    };

    if (data_len > sizeof(message.data)) {
        logf_error("Data length exceeds message data buffer size: %u", data_len);
        return false;
    }

    memcpy(message.data, data, data_len);
    if (extra_data_1) {
        message.extra_data[0].data = (uint8_t *) extra_data_1;
        message.extra_data[0].data_len = extra_data_1_len;
        message.extra_data[0].should_free_once_sent = should_free_once_sent;
    }

    if (queue_try_add(&node.message_queue, &message)) {
        node_link_node_signal_data_ready();
        return true;
    }
    return false;
}

static bool node_link_node_enqueue_simple_message(
    const node_link_node_msg_opcode_t opcode,
    const void *data,
    const size_t data_len
) {
    return node_link_node_enqueue_message(opcode, data, data_len, nullptr, 0, false);
}

bool node_link_node_enqueue_init() {
    const nl_node_msg_node_init_data_t message_data = {
    };

    return node_link_node_enqueue_simple_message(
        NL_NODE_MESSAGE_OP_INIT,
        &message_data,
        sizeof(message_data)
    );
}

bool node_link_node_enqueue_set_report(
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint8_t report_type,
    uint8_t const *report_data,
    const uint16_t report_data_len
) {
    const nl_node_msg_hid_set_report_data_t message_data = {
        .kvm_hid_idx = kvm_hid_idx,
        .report_id = report_id,
        .report_type = report_type,
    };

    uint8_t *malloced_report_data = malloc(report_data_len);
    if (malloced_report_data == nullptr) {
        logf_critical("Failed to allocate memory for report data: %u", report_data_len);
        return false;
    }
    memcpy(malloced_report_data, report_data, report_data_len);

    return node_link_node_enqueue_message(
        NL_NODE_MESSAGE_OP_SET_REPORT,
        &message_data,
        sizeof(message_data),
        malloced_report_data,
        report_data_len,
        true
    );
}

bool node_link_node_enqueue_set_hid_protocol(
    const uint8_t kvm_hid_idx,
    const uint8_t hid_protocol
) {
    const nl_node_msg_hid_set_hid_protocol_data_t message_data = {
        .kvm_hid_idx = kvm_hid_idx,
        .hid_protocol = hid_protocol,
    };

    return node_link_node_enqueue_simple_message(
        NL_NODE_MESSAGE_OP_SET_HID_PROTOCOL,
        &message_data,
        sizeof(message_data)
    );
}
