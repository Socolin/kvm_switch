#include "node_link_ctrl.h"

#include <math.h>
#include <string.h>

#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "computer_manager.h"
#include "gpio_utils.h"
#include "hid_manager.h"
#include "kvm_switch_controller.h"
#include "logger.h"
#include "node_link.h"

typedef struct {
    node_link_t link;
    bool node_ready[MAX_COMPUTER];
} node_link_ctrl_t;

static node_link_ctrl_t ctrl;

static void node_link_ctrl_process_received_message(const node_link_msg_t *message, void *udata);

void node_link_ctrl_init() {
    for (int i = 1; i < MAX_COMPUTER; i++) {
        const computer_t *computer = computer_manager_get_computer(i);

        gpio_init(computer->spi_selector_gpio);
        gpio_set_dir(computer->spi_selector_gpio, GPIO_OUT);

        gpio_init(computer->spi_ready_gpio);
        gpio_set_dir(computer->spi_ready_gpio, GPIO_IN);
        gpio_pull_down(computer->spi_ready_gpio);

        gpio_init(computer->data_available_gpio);
        gpio_set_dir(computer->data_available_gpio, GPIO_IN);
        gpio_pull_down(computer->data_available_gpio);
    }

    node_link_init_controller(&ctrl.link, spi0, node_link_ctrl_process_received_message);
}

#define RESTART_NODES_GPIO 15

static void node_link_ctrl_irq_handler(
    [[maybe_unused]] uint gpio,
    [[maybe_unused]] uint32_t event_mask,
    const void *user_data
) {
    const computer_t *computer = user_data;
    if (event_mask & GPIO_IRQ_EDGE_RISE) {
        ctrl.node_ready[computer->computer_id] = true;
    }
}

void node_link_ctrl_restart_nodes() {
    log_info("Restarting nodes to be ready");
    for (int i = 1; i < MAX_COMPUTER; i++) {
        const computer_t *computer = computer_manager_get_computer(i);
        ctrl.node_ready[i] = false;
        gpio_util_set_handler(computer->spi_ready_gpio, GPIO_IRQ_EDGE_RISE, node_link_ctrl_irq_handler, computer);
    }

    gpio_init(RESTART_NODES_GPIO);
    gpio_set_dir(RESTART_NODES_GPIO, GPIO_OUT);
    sleep_us(10);
    gpio_set_dir(RESTART_NODES_GPIO, GPIO_IN);

    log_info("Waiting for nodes to be ready..");
    for (int i = 1; i < MAX_COMPUTER; i++) {
        const uint64_t wait_start = time_us_64();
        while (!ctrl.node_ready[i]) {
            if (time_us_64() - wait_start > 3'000'000) {
                logf_error("Timeout waiting for node %d to be ready", i);
                break;
            }
            tight_loop_contents();
        }
    }
    for (int i = 1; i < MAX_COMPUTER; i++) {
        const computer_t *computer = computer_manager_get_computer(i);
        gpio_util_clear_handler(computer->spi_ready_gpio);
    }

    log_info("All nodes are ready");
}

static void node_link_ctrl_process_received_message(
    const node_link_msg_t *message,
    void *udata
) {
    logf_debug("opcode: %u, data_len: %u", message->header.opcode, message->header.data_len);
    const uint8_t computer_id = *(uint8_t *) udata;

    switch (message->header.opcode) {
        case NL_NODE_MESSAGE_OP_INIT: {
            computer_manager_init_computer(computer_id);
            node_link_ctrl_enqueue_send_init(computer_id);
            kvm_switch_controller_enqueue_computer_ready(computer_id);
            break;
        }
        case NL_NODE_MESSAGE_OP_SET_REPORT: {
            auto const message_data = (nl_node_msg_hid_set_report_data_t *) message->data;
            kvm_switch_controller_enqueue_computer_set_report(
                computer_id,
                message_data->kvm_hid_idx,
                message_data->report_id,
                message_data->report_type,
                message->extra_data[0].data,
                message->extra_data[0].data_len
            );
            break;
        }
        case NL_NODE_MESSAGE_OP_SET_HID_PROTOCOL: {
            auto const message_data = (nl_node_msg_hid_set_hid_protocol_data_t *) message->data;
            kvm_switch_controller_enqueue_computer_set_hid_protocol(
                computer_id,
                message_data->kvm_hid_idx,
                message_data->hid_protocol
            );
            break;
        }
        default: {
            logf_error("invalid opcode: %04x", message->header.opcode);
            break;
        }
    }
}


void node_link_ctrl_task() {
    for (uint8_t computer_id = 1; computer_id < MAX_COMPUTER; computer_id++) {
        computer_t *computer = computer_manager_get_computer(computer_id);

        const bool spi_ready = gpio_get(computer->spi_ready_gpio);
        if (!spi_ready) {
            continue;
        }

        node_link_spi_select_target(&ctrl.link, computer->spi_ready_gpio, computer->spi_selector_gpio);

        const bool data_available = gpio_get(computer->data_available_gpio);

        node_link_msg_t message;
        if (queue_try_peek(&computer->message_queue, &message)) {
            if (!node_link_send_message_blocking(&ctrl.link, &message, &computer_id)) {
                node_link_drain_buffer(&ctrl.link);
            } else {
                queue_remove_blocking(&computer->message_queue, nullptr);
                node_link_dispose_message(&message);
            }
        } else if (data_available) {
            if (!node_link_send_message_blocking(&ctrl.link, nullptr, &computer_id)) {
                node_link_drain_buffer(&ctrl.link);
            }
        }
    }
}


// ╔══════════════════════════════════╗
// ║     Enqueue message to send      ║
// ╚══════════════════════════════════╝

static bool node_link_ctrl_enqueue_message(
    const uint8_t computer_id,
    const node_link_ctrl_msg_opcode_t opcode,
    const void *data,
    const size_t data_len,
    const void *extra_data_1,
    const size_t extra_data_1_len,
    const bool should_free_once_sent
) {
    computer_t *computer = computer_manager_get_computer(computer_id);
    if (computer->state == COMPUTER_STATE_NOT_CONNECTED) {
        logf_warning("Node handling computer %u is not connected", computer_id);
        return false;
    }
    if (queue_is_full(&computer->message_queue)) {
        logf_warning("Message queue of computer %u is full", computer_id);
        return false;
    }

    logf_debug("opcode: %u, data_len: %u", opcode, data_len);

    node_link_msg_t message = {
        .header = {
            .opcode = opcode,
            .data_len = data_len,
            .extra_data_count = extra_data_1 ? 1 : 0,
        },
    };

    memcpy(message.data, data, data_len);

    if (extra_data_1) {
        message.extra_data[0].data = (uint8_t *) extra_data_1;
        message.extra_data[0].data_len = extra_data_1_len;
        message.extra_data[0].should_free_once_sent = should_free_once_sent;
    }

    return queue_try_add(&computer->message_queue, &message);
}

static bool node_link_ctrl_enqueue_simple_message(
    const uint8_t computer_id,
    const node_link_ctrl_msg_opcode_t opcode,
    const void *data,
    const size_t data_len
) {
    return node_link_ctrl_enqueue_message(computer_id, opcode, data, data_len, nullptr, 0, false);
}


void node_link_ctrl_enqueue_send_init(
    const uint8_t computer_id
) {
    const nl_ctrl_msg_init_data_t message_data = {
        .computer_id = computer_id,
    };

    node_link_ctrl_enqueue_simple_message(
        computer_id,
        NL_CTRL_MESSAGE_OP_INIT,
        &message_data,
        sizeof(message_data)
    );
}


void node_link_ctrl_enqueue_broadcast_hid_mount(
    const hid_t *hid
) {
    const nl_ctrl_msg_hid_mount_data_t message_data = {
        .dev_addr = hid->dev_addr,
        .host_hid_idx = hid->host_hid_idx,
        .kvm_hid_idx = hid->kvm_hid_idx,
        .itf_protocol = hid->itf_protocol,
        .vid = hid->vid,
        .pid = hid->pid,
    };

    for (uint8_t computer_id = 1; computer_id < MAX_COMPUTER; computer_id++) {
        node_link_ctrl_enqueue_message(computer_id,
                                       NL_CTRL_MESSAGE_OP_HID_MOUNT,
                                       &message_data,
                                       sizeof(message_data),
                                       hid->raw_report_descriptor,
                                       hid->raw_report_descriptor_len,
                                       false
        );
    }
}

void node_link_ctrl_enqueue_send_hid_mount(
    const uint8_t computer_id,
    const hid_t *hid
) {
    const nl_ctrl_msg_hid_mount_data_t message_data = {
        .dev_addr = hid->dev_addr,
        .host_hid_idx = hid->host_hid_idx,
        .kvm_hid_idx = hid->kvm_hid_idx,
        .itf_protocol = hid->itf_protocol,
        .vid = hid->vid,
        .pid = hid->pid,
    };

    node_link_ctrl_enqueue_message(computer_id,
                                   NL_CTRL_MESSAGE_OP_HID_MOUNT,
                                   &message_data,
                                   sizeof(message_data),
                                   hid->raw_report_descriptor,
                                   hid->raw_report_descriptor_len,
                                   false
    );
}

void node_link_ctrl_enqueue_broadcast_hid_umount(
    const uint8_t dev_addr,
    const uint8_t host_hid_idx
) {
    const nl_ctrl_msg_hid_umount_data_t message_data = {
        .dev_addr = dev_addr,
        .host_hid_idx = host_hid_idx,
    };

    for (uint8_t computer_id = 1; computer_id < MAX_COMPUTER; computer_id++) {
        node_link_ctrl_enqueue_simple_message(
            computer_id,
            NL_CTRL_MESSAGE_OP_HID_UMOUNT,
            &message_data,
            sizeof(message_data)
        );
    }
}

void node_link_ctrl_enqueue_send_start_usb_device(
    const uint8_t computer_id,
    const uint16_t vid,
    const uint16_t pid
) {
    const nl_ctrl_msg_start_usb_device_data_t message_data = {
        .vid = vid,
        .pid = pid,
    };

    node_link_ctrl_enqueue_simple_message(
        computer_id,
        NL_CTRL_MESSAGE_OP_START_USB_DEVICE,
        &message_data,
        sizeof(message_data)
    );
}

void node_link_ctrl_enqueue_broadcast_start_usb_device(
    const uint16_t vid,
    const uint16_t pid
) {
    const nl_ctrl_msg_start_usb_device_data_t message_data = {
        .vid = vid,
        .pid = pid,
    };

    for (uint8_t computer_id = 1; computer_id < MAX_COMPUTER; computer_id++) {
        node_link_ctrl_enqueue_simple_message(
            computer_id,
            NL_CTRL_MESSAGE_OP_START_USB_DEVICE,
            &message_data,
            sizeof(message_data)
        );
    }
}

void node_link_ctrl_enqueue_send_report(
    const uint8_t computer_id,
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint8_t *const report_data,
    const uint8_t report_data_length
) {
    nl_ctrl_msg_hid_report_data_t message_data = {
        .kvm_hid_idx = kvm_hid_idx,
        .report_id = report_id,
        .report_data_len = report_data_length,
    };

    if (report_data_length > sizeof(message_data.report_data)) {
        logf_critical("Report data length exceeds buffer size");
        return;
    }

    memcpy(message_data.report_data, report_data, report_data_length);

    node_link_ctrl_enqueue_simple_message(
        computer_id,
        NL_CTRL_MESSAGE_OP_HID_REPORT,
        &message_data,
        sizeof(message_data) - sizeof(message_data.report_data) + report_data_length
    );
}
