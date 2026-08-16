#include <math.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "hid_manager.h"
#include "internal_com.h"
#include "logger.h"
#include "bsp/board_api.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/util/queue.h"
#include "pio_usb.h"

// CH9350L documentation: https://wiki.kamamilabs.com/images/b/bf/CH9350DS.pdf

// Pin where the CH9350 is plugged
#define UART1_TX_PIN 4
#define UART1_RX_PIN 5

// Pin where the CH9329 is plugged
#define UART0_TX_PIN 1
#define UART0_RX_PIN 0

#ifndef BUILD_DATE
#define BUILD_DATE "No date"
#endif

static queue_t htc_msg_queue = {};
static queue_t cth_msg_queue = {};

static bool should_process_cth_messages = true;

void process_computer_to_hid_message() {
    if (!should_process_cth_messages)
        return;

    computer_to_hid_message_t cth_message;
    if (!queue_try_peek(&cth_msg_queue, &cth_message))
        return; // Nothing to do

    switch (cth_message.opcode) {
        case COMPUTER_TO_HID_SET_REPORT: {
            cth_message_set_report_t *data = (cth_message_set_report_t *) cth_message.data;
            should_process_cth_messages = false;
            if (!tuh_hid_set_report(
                data->dev_addr,
                data->itf_idx,
                data->report_id,
                data->report_type,
                data->buffer,
                data->buffer_len
            )) {
                logf_error("Failed to set report for device: %u interface: %u", data->dev_addr, data->itf_idx);
                return;
            }
            logf_info("Successfully set report for device: %u interface: %u", data->dev_addr, data->itf_idx);
            log_debug_hex_buffer(data->buffer, data->buffer_len);
            break;
        }
        case COMPUTER_TO_HID_SET_HID_PROTOCOL: {
            const cth_message_set_hid_protocol_t *data = (cth_message_set_hid_protocol_t *) cth_message.data;
            if (tuh_hid_get_protocol(data->dev_addr, data->itf_idx) == data->hid_protocol)
                break;
            should_process_cth_messages = false;
            if (!tuh_hid_set_protocol(data->dev_addr, data->itf_idx, data->hid_protocol)) {
                logf_error("Failed to set HID protocol for device %u interface %u to %u", data->dev_addr,
                       data->itf_idx, data->hid_protocol);
                return;
            }
            break;
        }
        default:
            logf_critical("Unknown computer to hid opcode %u", cth_message.opcode);
            break;
    }

    queue_try_remove(&cth_msg_queue, nullptr);
}

static void core1_main() {
    log_info("Starting USB Host on core 1");

    const pio_usb_configuration_t pio_cfg = {
        .pin_dp = 16,
        .pio_tx_num = PIO_USB_TX_DEFAULT,
        .sm_tx = PIO_SM_USB_TX_DEFAULT,
        .tx_ch = PIO_USB_DMA_TX_DEFAULT,
        .pio_rx_num = PIO_USB_RX_DEFAULT,
        .sm_rx = PIO_SM_USB_RX_DEFAULT,
        .sm_eop = PIO_SM_USB_EOP_DEFAULT,
        .alarm_pool = NULL,
        .debug_pin_rx = PIO_USB_DEBUG_PIN_NONE,
        .debug_pin_eop = PIO_USB_DEBUG_PIN_NONE
    };
    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    tuh_init(1);

    pio_usb_host_add_port(18, PIO_USB_PINOUT_DPDM);

    log_info("USB Host ready");

    while (true) {
        tuh_task();
        watchdog_update();
        process_computer_to_hid_message();
    }
}


void tuh_mount_cb(
    const uint8_t dev_addr
) {
    logf_debug("dev_addr: %u", dev_addr);

    const htc_message_device_mount_data_t message_data = {
        .dev_addr = dev_addr,
    };
    hid_to_computer_message_t htc_message = {
        .opcode = HID_TO_COMPUTER_DEVICE_MOUNT,
        .data_len = sizeof(message_data),
    };
    memcpy(&htc_message.data, &message_data, sizeof(message_data));
    queue_try_add(&htc_msg_queue, &htc_message);

    logf_info("Device %u is mounted", dev_addr);
}

void tuh_umount_cb(
    const uint8_t dev_addr
) {
    logf_debug("dev_addr: %u", dev_addr);

    const htc_message_device_umount_data_t message_data = {
        .dev_addr = dev_addr,
    };
    hid_to_computer_message_t htc_message = {
        .opcode = HID_TO_COMPUTER_DEVICE_UMOUNT,
        .data_len = sizeof(message_data),
    };
    memcpy(&htc_message.data, &message_data, sizeof(message_data));
    queue_try_add(&htc_msg_queue, &htc_message);

    logf_info("Device %u is unmounted", dev_addr);
}

void tuh_hid_mount_cb(
    const uint8_t dev_addr,
    const uint8_t idx,
    uint8_t const *report_desc,
    const uint16_t desc_len
) {
    logf_debug("dev_addr: %u, idx: %u", dev_addr, idx);
    log_debug_hex_buffer(report_desc, desc_len);

    uint16_t vid, pid;
    if (!tuh_vid_pid_get(dev_addr, &vid, &pid)) {
        logf_error("Could not get VID/PID for device: %u", dev_addr);
        return;
    }

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, idx);
    uint8_t *data_report_desc = malloc(desc_len);
    if (data_report_desc == NULL) {
        log_critical("Not enough memory to mount HID device");
        return;
    }

    memcpy(data_report_desc, report_desc, desc_len);
    const htc_message_hid_mount_data_t message_data = {
        .dev_addr = dev_addr,
        .itf_idx = idx,
        .report_desc = data_report_desc,
        .desc_len = desc_len,
        .itf_protocol = itf_protocol,
        .pid = pid,
        .vid = vid,
    };
    hid_to_computer_message_t htc_message = {
        .opcode = HID_TO_COMPUTER_HID_MOUNT,
        .data_len = sizeof(message_data),
    };

    memcpy(&htc_message.data, &message_data, sizeof(message_data));
    queue_try_add(&htc_msg_queue, &htc_message);

    if (!tuh_hid_receive_report(dev_addr, idx)) {
        log_error("tuh_hid_receive_report failed");
    }
}

void tuh_hid_set_report_complete_cb(
    const uint8_t dev_addr,
    const uint8_t idx,
    const uint8_t report_id,
    const uint8_t report_type,
    const uint16_t len
) {
    logf_debug("dev_addr: %u, idx: %u, report_id: %u, report_type: %u len: %u", dev_addr, idx, report_id, report_type,
               len);
    should_process_cth_messages = true;
}

void tuh_hid_set_protocol_complete_cb(
    const uint8_t dev_addr,
    const uint8_t idx,
    const uint8_t protocol
) {
    logf_debug("dev_addr: %u, idx: %u, protocol: %u", dev_addr, idx, protocol);
    should_process_cth_messages = true;
}

void tuh_hid_umount_cb(
    const uint8_t dev_addr,
    const uint8_t idx
) {
    logf_debug("dev_addr: %u, idx: %u", dev_addr, idx);
    const htc_message_hid_umount_data_t message_data = {
        .dev_addr = dev_addr,
        .idx = idx,
    };
    hid_to_computer_message_t htc_message = {
        .opcode = HID_TO_COMPUTER_HID_UMOUNT,
        .data_len = sizeof(message_data),
    };
    memcpy(&htc_message.data, &message_data, sizeof(message_data));
    queue_try_add(&htc_msg_queue, &htc_message);

    logf_info("HID device unmounted. dev_addr: %u, idx: %u", dev_addr, idx);
}


void tuh_hid_report_received_cb(
    const uint8_t dev_addr,
    const uint8_t idx,
    uint8_t const *report,
    const uint16_t len
) {
    logf_debug("dev_addr: %u, idx: %u", dev_addr, idx);
    log_debug_hex_buffer(report, len);

    if (len == 0) {
        log_warning("Received empty report");
        return;
    }

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, idx);
    const uint8_t hid_protocol = tuh_hid_get_protocol(dev_addr, idx);
    const bool use_report_id = hid_protocol == HID_PROTOCOL_REPORT && hid_mgr_is_hid_using_report_id(dev_addr, idx);

    htc_message_hid_report_data_t message_data = {
        .dev_addr = dev_addr,
        .idx = idx,
        .report_data_len = use_report_id ? len - 1 : len,
        .report_id = use_report_id ? report[0] : 0,
        .itf_protocol = itf_protocol,
        .hid_protocol = hid_protocol,
    };
    if (len > sizeof(message_data.report_data)) {
        logf_error("report data too long: %u", len);
        return;
    }
    memcpy(
        &message_data.report_data,
        use_report_id ? report + 1 : report,
        use_report_id ? len - 1 : len
    );

    hid_to_computer_message_t htc_message = {
        .opcode = HID_TO_COMPUTER_HID_REPORT,
        .data_len = sizeof(message_data),
    };
    memcpy(&htc_message.data, &message_data, sizeof(message_data));
    queue_try_add(&htc_msg_queue, &htc_message);

    if (!tuh_hid_receive_report(dev_addr, idx)) {
        log_error("tuh_hid_receive_report failed");
    }
}

int main() {
    watchdog_enable(5000, 1);

    stdio_init_all();

    logger_init(LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG);

    log_info("KVM is starting");
    logf_info("Version: %s", BUILD_DATE);

    // To use Pico-PIO-USB the system clock should be multiple of 12MHz.
    // USB requires a very precise clock and since PIO are working at a multiple
    // of the core frequency if it's not a multiple of 12MHz, some cycle will be longer or shorter and it will
    // create error when writing / reading usb.
    // FIXME: Can we update this for pico 2 ? like 144000
    set_sys_clock_khz(120000, true);

    sleep_ms(10);


    queue_init(&htc_msg_queue, sizeof(hid_to_computer_message_t), 32);
    queue_init(&cth_msg_queue, sizeof(computer_to_hid_message_t), 32);

    log_info("Initializing board");

    board_init();

    hid_mgr_init();

    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    board_init_after_tusb();
    uint64_t last_device_mounted = 0;

    log_debug("Starting main loop on core 0");

    while (true) {
        tud_task(); // tinyusb device task

        if (last_device_mounted) {
            const uint64_t now = time_us_64();
            // When a device is mounted, wait 1 second before setting up the pico as a usb device.
            // This allow to avoid multiple re-initializations of the usb device each time.
            // FIXME: Can probably be done immediately after the second device is mounted. or other
            if (now - last_device_mounted > 1'000'000) {
                last_device_mounted = 0;
                if (tud_inited()) {
                    log_info("Re-initializing USB device");
                    tud_deinit(BOARD_TUD_RHPORT);
                } else {
                    log_info("Initializing USB device");
                }

                // init device stack on configured roothub port
                const tusb_rhport_init_t rh_init = {
                    .role = TUSB_ROLE_DEVICE,
                    .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL
                };
                if (!tud_rhport_init(BOARD_TUD_RHPORT, &rh_init)) {
                    log_error("Failed to initialize USB device");
                    return 0;
                }
            }
        }

        hid_to_computer_message_t htc_message;
        if (queue_try_remove(&htc_msg_queue, &htc_message)) {
            switch (htc_message.opcode) {
                case HID_TO_COMPUTER_HID_MOUNT: {
                    const htc_message_hid_mount_data_t *data = (htc_message_hid_mount_data_t *) htc_message.data;
                    if (!hid_mgr_register_hid(
                            data->dev_addr,
                            data->itf_idx,
                            data->itf_protocol,
                            data->report_desc,
                            data->desc_len)
                    ) {
                        free(data->report_desc);
                    }
                    break;
                }
                case HID_TO_COMPUTER_DEVICE_MOUNT: {
                    last_device_mounted = time_us_64();
                    break;
                }
                case HID_TO_COMPUTER_HID_UMOUNT: {
                    const htc_message_hid_umount_data_t *data = (htc_message_hid_umount_data_t *) htc_message.data;
                    hid_mgr_unregister_hid(data->dev_addr, data->idx);
                    break;
                }
                case HID_TO_COMPUTER_DEVICE_UMOUNT: {
                    break;
                }
                case HID_TO_COMPUTER_HID_REPORT: {
                    const htc_message_hid_report_data_t *data = (htc_message_hid_report_data_t *) htc_message.data;
                    // FIXME: make this configurable
                    if (data->report_data_len > 2 && data->report_data[2] == 0x48 && data->itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
                        log_critical("Resetting PICO in BOOTSEL");
                        multicore_reset_core1();
                        reset_usb_boot(0, 0);
                    }
                    if (!tud_hid_ready()) {
                        log_warning("tud_hid_ready == false, skip report");
                        continue;
                    }
                    const uint8_t interface_idx = hid_mgr_get_hid_idx(data->dev_addr, data->idx);

                    if (!tud_hid_n_report(interface_idx, data->report_id, data->report_data, data->report_data_len)) {
                        log_error("tud_hid_n_report failed");
                    }
                    break;
                }
                default:
                    // FIXME: error
                    break;
            }
        }
        watchdog_update();
    }
}

void tud_mount_cb() {
    logf_debug("device mounted");

    for (uint8_t i = 0; i < hid_mgr_get_max_hid_count(); i++) {
        const hid_t *hid = hid_mgr_get(i);
        if (hid == nullptr) {
            continue;
        }
        // Only keyboard and mouse support boot protocol
        if (hid->interface_protocol == HID_ITF_PROTOCOL_NONE)
            continue;

        const uint8_t hid_protocol = tud_hid_n_get_protocol(i);
        logf_debug("HID protocol for device: %u interface: %u is: %u", hid->dev_addr, hid->interface_idx, hid_protocol);
        const cth_message_set_hid_protocol_t message_data = {
            .dev_addr = hid->dev_addr,
            .itf_idx = hid->interface_idx,
            .hid_protocol = hid_protocol,
        };
        computer_to_hid_message_t cth_message = {
            .opcode = COMPUTER_TO_HID_SET_HID_PROTOCOL,
            .data_len = sizeof(message_data),
        };
        memcpy(cth_message.data, &message_data, sizeof(message_data));
        queue_try_add(&cth_msg_queue, &cth_message);
    }
}

uint16_t tud_hid_get_report_cb(
    const uint8_t instance,
    const uint8_t report_id,
    const hid_report_type_t report_type,
    uint8_t *buffer,
    const uint16_t reqlen
) {
    logf_debug("instance: %u, report_id: %u, report_type: %u", instance, report_id, report_type);

    (void) buffer;
    (void) reqlen;

    return 0;
}


void tud_hid_set_report_cb(
    const uint8_t instance,
    const uint8_t report_id,
    const hid_report_type_t report_type,
    uint8_t const *buffer,
    const uint16_t bufsize
) {
    logf_debug("instance: %u, report_id: %u, report_type: %u", instance, report_id, report_type);
    log_debug_hex_buffer(buffer, bufsize);

    const hid_t *hid = hid_mgr_get(instance);
    if (hid == nullptr)
        return;

    cth_message_set_report_t message_data = {
        .dev_addr = hid->dev_addr,
        .itf_idx = hid->interface_idx,
        .report_type = report_type,
        .report_id = report_id,
        .buffer_len = bufsize,
    };
    if (bufsize > sizeof(message_data.buffer)) {
        log_error("bufsize is too large");
        return;
    }
    memcpy(message_data.buffer, buffer, bufsize);
    computer_to_hid_message_t cth_message = {
        .opcode = COMPUTER_TO_HID_SET_REPORT,
        .data_len = sizeof(message_data),
    };
    memcpy(cth_message.data, &message_data, sizeof(message_data));
    queue_try_add(&cth_msg_queue, &cth_message);
}

void tud_hid_set_protocol_cb(
    const uint8_t instance,
    const uint8_t protocol
) {
    logf_debug("instance:%u protocol:%u", instance, protocol);

    const hid_t *hid = hid_mgr_get(instance);
    if (hid == nullptr)
        return;

    const cth_message_set_hid_protocol_t message_data = {
        .dev_addr = hid->dev_addr,
        .itf_idx = hid->interface_idx,
        .hid_protocol = protocol,
    };
    computer_to_hid_message_t cth_message = {
        .opcode = COMPUTER_TO_HID_SET_HID_PROTOCOL,
        .data_len = sizeof(message_data),
    };
    memcpy(cth_message.data, &message_data, sizeof(message_data));
    queue_try_add(&cth_msg_queue, &cth_message);
}
