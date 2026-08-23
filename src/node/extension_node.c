#include "extension_node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <machine/endian.h>

#include "kvm_switch_node.h"
#include "../shared/crc.h"
#include "../shared/utils.h"
#include "../shared/extension_messages.h"
#include "../shared/logger.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/util/queue.h"

typedef struct {
    extension_message_transport_header_t empty_transport_header;
    /**
     * We need a buffer large enough to a full report_descriptor that can be a maximum theoretically of 65536 bytes
     * This is unlikely to be reached in practice, but it's a safe upper bound, if we need more memory at some point
     * this can probably be reduced
     */
    uint8_t rx_buffer[65536 + 256];
    uint8_t tx_buffer[65536 + 256];
    spi_inst_t *spi;
    queue_t message_queue;
} extension_node_t;

static extension_node_t node = {
    .empty_transport_header = {
        .header = EXTENSION_MESSAGE_HEADER,
        .message_len = 0,
        .protocol_opcode = EXTENSION_PROTOCOL_OPCODE_EMPTY,
        .message_crc = 0,
        .header_crc = 0,
    },
};

#define SPI_READY_GPIO 6
#define DATA_READY_GPIO 7
// FIXME: move to shared
static uint16_t compute_transport_crc(
    const extension_message_transport_header_t *transport_header
) {
    return crc16_ccitt_false(
        (uint8_t *) transport_header,
        sizeof(*transport_header) - sizeof(transport_header->header_crc)
    );
}

void extension_node_init() {
    queue_init(&node.message_queue, sizeof(extension_message_t), 16);

    spi_init(spi0, 10'000'000);

    gpio_set_function(2, GPIO_FUNC_SPI);
    gpio_set_function(3, GPIO_FUNC_SPI);
    gpio_set_function(4, GPIO_FUNC_SPI);
    gpio_set_function(5, GPIO_FUNC_SPI);

    gpio_init(SPI_READY_GPIO);
    gpio_set_dir(SPI_READY_GPIO, GPIO_OUT);
    gpio_init(DATA_READY_GPIO);
    gpio_set_dir(DATA_READY_GPIO, GPIO_OUT);

    node.spi = spi0;
    node.empty_transport_header.message_crc = crc16_ccitt_false(nullptr, 0);
    node.empty_transport_header.header_crc = compute_transport_crc(&node.empty_transport_header);
}

static void extension_node_spi_ready() {
    gpio_put(SPI_READY_GPIO, 1);
}

static void extension_node_spi_not_ready() {
    gpio_put(SPI_READY_GPIO, 0);
}

static void extension_node_data_ready() {
    gpio_put(DATA_READY_GPIO, 1);
}

static void extension_node_data_not_ready() {
    gpio_put(DATA_READY_GPIO, 0);
}

static bool extension_node_process_received_message(
    const extension_message_transport_header_t *rx_transport_header
) {
    if (rx_transport_header->protocol_opcode == EXTENSION_PROTOCOL_OPCODE_EMPTY) {
        return true;
    }

    const uint16_t message_crc = crc16_ccitt_false(node.rx_buffer, rx_transport_header->message_len);
    if (message_crc != rx_transport_header->message_crc) {
        logf_error("invalid crc actual: %04x expected: %04x", message_crc, rx_transport_header->message_crc);
        return false;
    }

    log_debug_hex_buffer(node.rx_buffer, rx_transport_header->message_len);

    const extension_message_header_t *rx_message_header = (extension_message_header_t *) node.rx_buffer;
    const uint8_t *data_start = node.rx_buffer + sizeof(extension_message_header_t);
    const uint8_t *extra_data_start = data_start + rx_message_header->data_len;

    switch (rx_message_header->opcode) {
        case EXTENSION_HOST_MESSAGE_OP_HID_MOUNT: {
            const extension_message_hid_mount_data_t *message_data = (extension_message_hid_mount_data_t *) data_start;
            const uint16_t report_desc_len = *extra_data_start;
            uint8_t *report_desc = malloc(report_desc_len);
            if (report_desc == nullptr) {
                log_critical("failed to allocate memory for report descriptor");
            }
            memcpy(report_desc, extra_data_start + sizeof(uint16_t), report_desc_len);

            kvm_switch_node_enqueue_hid_mount(
                message_data->dev_addr,
                message_data->host_hid_idx,
                message_data->kvm_hid_idx,
                message_data->itf_protocol,
                message_data->vid,
                message_data->pid,
                report_desc,
                report_desc_len
            );
            break;
        }
        case EXTENSION_HOST_MESSAGE_OP_HID_UMOUNT: {
            const extension_message_hid_umount_data_t *message_data = (extension_message_hid_umount_data_t *)
                    data_start;
            kvm_switch_node_enqueue_hid_umount(message_data->dev_addr, message_data->host_hid_idx);
            break;
        }
        case EXTENSION_HOST_MESSAGE_OP_HID_REPORT: {
            const extension_message_hid_report_data_t *message_data = (extension_message_hid_report_data_t *)
                    data_start;
            kvm_switch_node_enqueue_hid_report(
                message_data->kvm_hid_idx,
                message_data->report_id,
                message_data->report_data_len,
                message_data->report_data
            );
            break;
        }
        case EXTENSION_HOST_MESSAGE_OP_START_USB_DEVICE: {
            kvm_switch_node_enqueue_connect_usb_device();
            break;
        }
        default:
            logf_error("invalid opcode: %04x", rx_message_header->opcode);
            return false;
    }
    // FIXME: enqueue packet to kvm_node
    return true;
}

static bool extension_node_send_ack(
    const bool ack
) {
    extension_message_transport_header_t rx_transport_header;
    extension_message_transport_header_t tx_transport_header = {
        .header = EXTENSION_MESSAGE_HEADER,
        .protocol_opcode = ack ? EXTENSION_PROTOCOL_OPCODE_ACK : EXTENSION_PROTOCOL_OPCODE_NACK,
        .message_len = 0,
        .message_crc = crc16_ccitt_false(nullptr, 0),
    };
    tx_transport_header.header_crc = compute_transport_crc(&tx_transport_header);

    extension_node_data_ready();
    extension_node_spi_ready();
    spi_write_read_blocking(
        node.spi,
        (const uint8_t *) &tx_transport_header,
        (uint8_t *) &rx_transport_header,
        sizeof(rx_transport_header)
    );
    extension_node_spi_not_ready();
    extension_node_data_not_ready();

    const uint8_t header_crc = compute_transport_crc(&rx_transport_header);
    if (header_crc != rx_transport_header.header_crc)
        return false;

    return tx_transport_header.protocol_opcode == EXTENSION_PROTOCOL_OPCODE_ACK;
}

static void extension_node_wait_for_controller_data_blocking() {
    extension_message_transport_header_t rx_transport_header;

    extension_node_spi_ready();

    spi_write_read_blocking(
        node.spi,
        (const uint8_t *) &node.empty_transport_header,
        (uint8_t *) &rx_transport_header,
        sizeof(rx_transport_header)
    );

    spi_read_blocking(
        node.spi,
        0x00,
        node.rx_buffer,
        rx_transport_header.message_len
    );

    extension_node_spi_not_ready();

    const bool successfully_received = extension_node_process_received_message(&rx_transport_header);

    extension_node_send_ack(successfully_received);
}

static void copy_message_to_send_buffer(
    const extension_message_t *message
) {
    memcpy(node.tx_buffer, &message->header, sizeof(message->header));
    memcpy(node.tx_buffer, message->data, message->header.data_len);
    size_t total_data_len = message->header.data_len;
    for (uint8_t i = 0; i < message->header.extra_data_count; i++) {
        const extension_message_extra_data_t *extra_data = &message->extra_data[i];
        memcpy(node.tx_buffer + total_data_len, &extra_data->data_len, sizeof(extra_data->data_len));
        total_data_len += sizeof(extra_data->data_len);
        memcpy(node.tx_buffer + total_data_len, extra_data->data, extra_data->data_len);
        total_data_len += extra_data->data_len;
    }
}

static bool extension_node_send_data_blocking(
    const extension_message_t *message
) {
    extension_message_transport_header_t rx_transport_header;

    copy_message_to_send_buffer(message);

    extension_node_data_ready();
    extension_node_spi_ready();
    spi_write_read_blocking(
        node.spi,
        (const uint8_t *) &message->transport_header,
        (uint8_t *) &rx_transport_header,
        sizeof(rx_transport_header)
    );

    spi_write_read_blocking(
        node.spi,
        node.tx_buffer,
        node.rx_buffer,
        max32(rx_transport_header.message_len, message->transport_header.message_len)
    );

    extension_node_spi_not_ready();
    extension_node_data_not_ready();

    const bool successfully_received = extension_node_process_received_message(&rx_transport_header);

    extension_node_send_ack(successfully_received);

    return true;
}

void extension_node_run() {
    while (true) {
        extension_message_t message;
        if (!queue_try_peek(&node.message_queue, &message)) {
            // Nothing to send
            extension_node_wait_for_controller_data_blocking();
        } else {
            if (extension_node_send_data_blocking(&message)) {
                queue_remove_blocking(&node.message_queue, nullptr);
                for (uint8_t i = 0; i < message.header.extra_data_count; i++) {
                    if (message.extra_data[i].should_free_once_sent) {
                        free(message.extra_data[i].data);
                    }
                }
            }
        }
    }
}


// ╔══════════════════════════════════╗
// ║     Enqueue message to send      ║
// ╚══════════════════════════════════╝

static bool extension_node_enqueue_message(
    const extension_node_message_opcode_t opcode,
    const void *data,
    const size_t data_len,
    const void *extra_data_1,
    const size_t extra_data_1_len,
    const bool should_free_once_sent
) {
    logf_debug("opcode: %u, data_len: %u", opcode, data_len);

    extension_message_t message = {
        .transport_header = {
            .header = EXTENSION_MESSAGE_HEADER,
            .message_len = sizeof(message.header) + data_len + extra_data_1_len,
            .message_crc = 0,
            .header_crc = 0,
        },
        .header = {
            .opcode = opcode,
            .data_len = data_len,
            .extra_data_count = extra_data_1 ? 1 : 0,
        },
    };

    memcpy(message.data, data, data_len);

    uint16_t message_crc = crc16_ccitt_false((uint8_t *) &message.header, sizeof(message.header));
    message_crc = crc16_ccitt_false_continue(message_crc, message.data, data_len);

    if (extra_data_1) {
        message.extra_data[0].data = (uint8_t *) extra_data_1;
        message.extra_data[0].data_len = extra_data_1_len;
        message.extra_data[0].should_free_once_sent = should_free_once_sent;

        message_crc = crc16_ccitt_false_continue(message_crc, (uint8_t *) &extra_data_1_len, sizeof(extra_data_1_len));
        message_crc = crc16_ccitt_false_continue(message_crc, extra_data_1, extra_data_1_len);
    }
    message.transport_header.message_crc = message_crc;
    message.transport_header.header_crc = compute_transport_crc(&message.transport_header);

    return queue_try_add(&node.message_queue, &message);
}

static bool extension_node_enqueue_simple_message(
    const extension_node_message_opcode_t opcode,
    const void *data,
    const size_t data_len
) {
    return extension_node_enqueue_message(opcode, data, data_len, nullptr, 0, false);
}

bool extension_node_enqueue_set_report(
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint8_t report_type,
    uint8_t const *report_data,
    const uint16_t report_data_len
) {
    const extension_message_hid_set_report_data_t message_data = {
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

    return extension_node_enqueue_message(
        EXTENSION_NODE_MESSAGE_OP_SET_REPORT,
        &message_data,
        sizeof(message_data),
        malloced_report_data,
        report_data_len,
        true
    );
}

bool extension_node_enqueue_set_hid_protocol(
    const uint8_t kvm_hid_idx,
    const uint8_t hid_protocol
) {
    const extension_message_hid_set_hid_protocol_data_t message_data = {
        .kvm_hid_idx = kvm_hid_idx,
        .hid_protocol = hid_protocol,
    };

    return extension_node_enqueue_simple_message(
        EXTENSION_NODE_MESSAGE_OP_SET_HID_PROTOCOL,
        &message_data,
        sizeof(message_data)
    );
}
