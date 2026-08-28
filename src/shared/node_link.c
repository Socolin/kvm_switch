#include "node_link.h"

#include <stdlib.h>
#include <string.h>

#include "crc.h"
#include "logger.h"
#include "utils.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

static uint16_t node_link_compute_transport_crc(const node_link_transport_header_t *transport_header);

static void node_link_init(
    node_link_t *link,
    spi_inst_t *spi
) {
    spi_init(spi, NODE_LINK_SPI_BAUD_RATE);
    spi_set_format(spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    spi_set_slave(spi, !link->is_controller);
    link->spi = spi;
    link->baud_rate = NODE_LINK_SPI_BAUD_RATE;

    memset(link->drain_buffer, 0, sizeof(link->drain_buffer));

    gpio_set_function(2, GPIO_FUNC_SPI);
    gpio_set_function(3, GPIO_FUNC_SPI);
    gpio_set_function(4, GPIO_FUNC_SPI);
    gpio_set_function(5, GPIO_FUNC_SPI);
}

void node_link_init_controller(
    node_link_t *link,
    spi_inst_t *spi,
    const message_handler_t message_handler
) {
    link->is_controller = true;
    link->spi_ready_gpio = -1;
    link->spi_select_gpio = -1;
    link->message_handler = message_handler;
    node_link_init(link, spi);
}

void node_link_init_node(
    node_link_t *link,
    spi_inst_t *spi,
    const uint8_t spi_ready_gpio,
    const message_handler_t message_handler
) {
    link->is_controller = false;
    link->spi_ready_gpio = spi_ready_gpio;
    link->spi_select_gpio = -1;
    link->message_handler = message_handler;
    node_link_init(link, spi);
}

void node_link_spi_select_target(
    node_link_t *link,
    const uint8_t spi_ready_gpio,
    const uint8_t spi_select_gpio
) {
    link->spi_ready_gpio = spi_ready_gpio;
    link->spi_select_gpio = spi_select_gpio;
}

static bool wait_for_spi_ready(uint8_t gpio) {
    // Wait a few cycles to the node to update the gpio state
    asm volatile("nop \n nop \n nop \n nop \n nop \n nop");

    uint64_t wait_start = time_us_64();

    while (!gpio_get(gpio)) {
        if (time_us_64() - wait_start > MAX_WAIT_FOR_SPI_READY_US) {
            logf_warning("spi_ready_gpio waited too long, gpio %u", gpio);
            return false;
        }
    }

    asm volatile("nop \n nop \n nop \n nop \n nop \n nop");
    return true;
}

static __inline__ void node_link_start_transaction(
    const node_link_t *link
) {
    gpio_put(link->spi_select_gpio, 1);
    asm volatile("nop \n nop \n nop \n nop \n nop \n nop");
}

static __inline__ void node_link_end_transaction(
    const node_link_t *link
) {
    gpio_put(link->spi_select_gpio, 0);
    asm volatile("nop \n nop \n nop \n nop \n nop \n nop");
}

static __inline__ void node_link_signal_ready(
    const node_link_t *link
) {
    gpio_put(link->spi_ready_gpio, 1);
}

static __inline__ void node_link_clear_ready(
    const node_link_t *link
) {
    gpio_put(link->spi_ready_gpio, 0);
}

static bool node_link_write_read_blocking(
    const node_link_t *link,
    const uint8_t *tx_buffer,
    uint8_t *rx_buffer,
    const size_t len
) {
    // Drain any pending data from the SPI RX FIFO.
    // Sometimes it seems some bytes are left and it's breaking all following transactions. (not sure what is happening)
    node_link_drain_rx(link);

    if (link->is_controller) {
        if (!wait_for_spi_ready(link->spi_ready_gpio))
            return false;
        node_link_start_transaction(link);
    } else {
        node_link_signal_ready(link);
    }

    spi_write_read_blocking(
        link->spi,
        tx_buffer,
        rx_buffer,
        len
    );

    if (link->is_controller) {
        node_link_end_transaction(link);
    } else {
        node_link_clear_ready(link);
    }

    return true;
}

static bool node_link_exchange_header(
    const node_link_t *link,
    const node_link_transport_header_t *tx_header,
    node_link_transport_header_t *rx_header
) {
    if (!node_link_write_read_blocking(
        link,
        (const uint8_t *) tx_header,
        (uint8_t *) rx_header,
        sizeof(*tx_header)
    )) {
        log_warning("node_link_write_read_blocking failed");
        return false;
    }

    if (rx_header->header != NL_MESSAGE_HEADER) {
        logf_warning("Invalid transport header: %02x expected: %02x", rx_header->header, NL_MESSAGE_HEADER);
        log_warning_hex_buffer(rx_header, sizeof(*rx_header));
        log_warning_hex_buffer(tx_header, sizeof(*rx_header));
        return false;
    }

    const uint16_t crc = node_link_compute_transport_crc(rx_header);
    if (crc != rx_header->header_crc) {
        logf_warning("Invalid transport header actual: %04x expected: %04x", crc, rx_header->header_crc);
        log_warning_hex_buffer(rx_header, sizeof(*rx_header));
        return false;
    }

    return true;
}

static uint32_t node_link_prepare_tx_buffer(
    node_link_t *link,
    const node_link_msg_t *message
) {
    if (message == nullptr)
        return 0;

    if (sizeof(message->header) + message->header.data_len > sizeof(link->tx_buffer)) {
        logf_error("Data size exceeding tx_buffer size. data_len=%u, tx_buffer_size=%zu",
                   message->header.data_len,
                   sizeof(link->tx_buffer)
        );
        return 0;
    }

    uint32_t message_length = 0;
    memcpy(link->tx_buffer, &message->header, sizeof(message->header));

    message_length += sizeof(message->header);
    memcpy(link->tx_buffer + message_length, message->data, message->header.data_len);

    message_length += message->header.data_len;
    for (uint8_t i = 0; i < message->header.extra_data_count; i++) {
        const node_link_msg_extra_data_t *extra_data = &message->extra_data[i];

        if (message_length + sizeof(extra_data->data_len) + extra_data->data_len > sizeof(link->tx_buffer)) {
            logf_error("Extra data size exceeding tx_buffer size. data_len=%u, tx_buffer_size=%zu",
                       message->header.data_len,
                       sizeof(link->tx_buffer)
            );
            return 0;
        }

        memcpy(link->tx_buffer + message_length, &extra_data->data_len, sizeof(extra_data->data_len));
        message_length += sizeof(extra_data->data_len);
        memcpy(link->tx_buffer + message_length, extra_data->data, extra_data->data_len);
        message_length += extra_data->data_len;
    }
    return message_length;
}

static bool node_link_map_rx_buffer_to_message(
    node_link_t *link,
    node_link_msg_t *message
) {
    size_t message_length = 0;

    memcpy(&message->header, link->rx_buffer, sizeof(message->header));
    message_length += sizeof(message->header);

    if (message_length + message->header.data_len > sizeof(link->rx_buffer)) {
        logf_warning("Data size exceeding rx_buffer size. data_len=%u, rx_buffer_size=%zu",
                     message->header.data_len,
                     sizeof(link->rx_buffer)
        );
        return false;
    }

    memcpy(message->data, link->rx_buffer + message_length, message->header.data_len);
    message_length += message->header.data_len;

    for (uint8_t i = 0; i < message->header.extra_data_count; i++) {
        node_link_msg_extra_data_t *extra_data = &message->extra_data[i];

        if (message_length + sizeof(extra_data->data_len) > sizeof(link->rx_buffer)) {
            logf_warning("Extra data size header exceeding rx_buffer size. message_length=%u", message_length);
            return false;
        }

        memcpy(&extra_data->data_len, link->rx_buffer + message_length, sizeof(extra_data->data_len));
        message_length += sizeof(extra_data->data_len);

        if (message_length + extra_data->data_len > sizeof(link->rx_buffer)) {
            logf_warning("Extra data size exceeding rx_buffer size. data_len=%u, message_length=%zu",
                         extra_data->data_len,
                         message_length
            );
            return false;
        }

        extra_data->data = link->rx_buffer + message_length;
        extra_data->should_free_once_sent = false;
        message_length += extra_data->data_len;
    }

    return true;
}

static void node_link_init_data_header(
    node_link_transport_header_t *header,
    const uint32_t message_len,
    const uint16_t message_crc
) {
    header->header = NL_MESSAGE_HEADER;
    header->protocol_opcode = NL_TRANSPORT_OPCODE_DATA;
    header->message_len = message_len;
    header->message_crc = message_crc;
    header->header_crc = node_link_compute_transport_crc(header);
}

static void node_link_init_ack_header(
    node_link_transport_header_t *header,
    const bool ack
) {
    header->header = NL_MESSAGE_HEADER;
    header->protocol_opcode = ack ? NL_TRANSPORT_OPCODE_ACK : NL_TRANSPORT_OPCODE_NACK;
    header->message_len = 0;
    header->message_crc = crc16_ccitt_false(nullptr, 0);
    header->header_crc = node_link_compute_transport_crc(header);
}

static bool node_link_exchange_ack(
    const node_link_t *link,
    const bool ack
) {
    node_link_transport_header_t rx_header;
    node_link_transport_header_t tx_header;

    node_link_init_ack_header(&tx_header, ack);

    if (!node_link_exchange_header(link, &tx_header, &rx_header)) {
        log_warning("node_link_exchange_header failed");
        return false;
    }

    return rx_header.protocol_opcode == NL_TRANSPORT_OPCODE_ACK;
}

bool node_link_send_message_blocking(
    node_link_t *link,
    const node_link_msg_t *tx_message,
    void *udata
) {
    const uint32_t tx_message_len = node_link_prepare_tx_buffer(link, tx_message);
    const uint16_t tx_message_crc = crc16_ccitt_false(link->tx_buffer, tx_message_len);

    node_link_transport_header_t tx_header;
    node_link_init_data_header(&tx_header, tx_message_len, tx_message_crc);

    node_link_transport_header_t rx_header;
    if (!node_link_exchange_header(link, &tx_header, &rx_header)) {
        log_warning("node_link_exchange_header failed");
        return false;
    }

    if (rx_header.protocol_opcode != NL_TRANSPORT_OPCODE_DATA) {
        logf_warning("Invalid protocol opcode. Expected data but was: %u", rx_header.protocol_opcode);
        return false;
    }

    const uint32_t exchange_len = max32(rx_header.message_len, tx_header.message_len);
    if (exchange_len > sizeof(link->rx_buffer)) {
        logf_warning("Invalid message_len. Too long: %lu", exchange_len);
        return false;
    }

    if (!node_link_write_read_blocking(
            link,
            link->tx_buffer,
            link->rx_buffer,
            exchange_len)
    ) {
        log_warning("node_link_write_read_blocking failed");
        return false;
    }

    const uint16_t rx_message_crc = crc16_ccitt_false(link->rx_buffer, rx_header.message_len);
    if (rx_message_crc != rx_header.message_crc) {
        node_link_exchange_ack(link, 0);
        logf_error("Invalid message CRC. actual: %04x expected: %04x", rx_message_crc, rx_header.message_crc);
        log_error_hex_buffer(link->rx_buffer, rx_header.message_len);
        return false;
    }

    node_link_exchange_ack(link, 1);

    if (rx_header.message_len > 0) {
        node_link_msg_t rx_message;
        if (node_link_map_rx_buffer_to_message(link, &rx_message)) {
            link->message_handler(&rx_message, udata);
        }
    }

    return true;
}


static uint16_t node_link_compute_transport_crc(
    const node_link_transport_header_t *transport_header
) {
    return crc16_ccitt_false(
        (uint8_t *) transport_header,
        sizeof(*transport_header) - sizeof(transport_header->header_crc)
    );
}

/**
 * This is used to force the node that may be blocked in a read operation to exit it.
 * After an error the node side will wait long enough for this to be completed.
 */

void node_link_drain_buffer(
    const node_link_t *link
) {
    node_link_drain_rx(link);

    // If this gpio is not set, the node is not reading
    if (!gpio_get(link->spi_ready_gpio)) {
        return;
    }

    node_link_start_transaction(link);
    while (gpio_get(link->spi_ready_gpio))
        spi_write_blocking(link->spi, link->drain_buffer, sizeof(link->drain_buffer));
    node_link_end_transaction(link);

    log_warning("buffer drained");
}

void node_link_drain_rx(
    const node_link_t *link
) {
    uint8_t b;
    while (spi_is_readable(link->spi))
        spi_read_blocking(link->spi, 0, &b, 1);
}

uint64_t node_link_get_us_delay_before_retry(
    const node_link_t *link
) {
    constexpr uint64_t bit_count = sizeof(link->tx_buffer) * 8;
    constexpr uint64_t us = 1'000'000;
    return MAX_WAIT_FOR_SPI_READY_US + bit_count * us / link->baud_rate;
}

void node_link_dispose_message(
    const node_link_msg_t *message
) {
    for (uint8_t i = 0; i < message->header.extra_data_count; i++) {
        if (message->extra_data[i].should_free_once_sent) {
            free(message->extra_data[i].data);
        }
    }
}
