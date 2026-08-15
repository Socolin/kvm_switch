#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "hid_manager.h"
#include "internal_com.h"
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
            if (!tuh_hid_set_report(
                data->dev_addr,
                data->itf_idx,
                data->report_id,
                data->report_type,
                data->buffer,
                data->buffer_len
            )) {
                printf("Error: failed to set report for device %u interface %u\n", data->dev_addr, data->itf_idx);
                should_process_cth_messages = false;
                return;
            }
            printf("Successfully set report for device %u interface %u\n", data->dev_addr, data->itf_idx);
            debug_print_buffer(data->buffer, data->buffer_len);
            break;
        }
        case COMPUTER_TO_HID_SET_HID_PROTOCOL: {
            const cth_message_set_hid_protocol_t *data = (cth_message_set_hid_protocol_t *) cth_message.data;
            if (tuh_hid_get_protocol(data->dev_addr, data->itf_idx) == data->hid_protocol)
                break;
            if (!tuh_hid_set_protocol(data->dev_addr, data->itf_idx, data->hid_protocol)) {
                printf("Error: failed to set HID protocol for device %u interface %u to %u\n", data->dev_addr,
                       data->itf_idx, data->hid_protocol);
                should_process_cth_messages = false;
                return;
            }
            break;
        }
        default:
            printf("Error: unknown computer to hid opcode %u\n", cth_message.opcode);
            break;
    }

    queue_try_remove(&cth_msg_queue, nullptr);
}

static void core1_main() {
    // sleep_ms(1000);

    printf("KVM starting %s\n", BUILD_DATE);

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

    printf("KVM ready\n");

    while (true) {
        tuh_task();
        watchdog_update();
        process_computer_to_hid_message();
    }
}


void tuh_mount_cb(
    const uint8_t dev_addr
) {
    const htc_message_device_mount_data_t message_data = {
        .dev_addr = dev_addr,
    };
    hid_to_computer_message_t htc_message = {
        .opcode = HID_TO_COMPUTER_DEVICE_MOUNT,
        .data_len = sizeof(message_data),
    };
    memcpy(&htc_message.data, &message_data, sizeof(message_data));
    queue_try_add(&htc_msg_queue, &htc_message);

    printf("[%u] Device is mounted\n", dev_addr);
}

void tuh_umount_cb(
    const uint8_t dev_addr
) {
    const htc_message_device_umount_data_t message_data = {
        .dev_addr = dev_addr,
    };
    hid_to_computer_message_t htc_message = {
        .opcode = HID_TO_COMPUTER_DEVICE_UMOUNT,
        .data_len = sizeof(message_data),
    };
    memcpy(&htc_message.data, &message_data, sizeof(message_data));
    queue_try_add(&htc_msg_queue, &htc_message);

    printf("[%u] Device is unmounted\n", dev_addr);
}

void tuh_hid_mount_cb(
    const uint8_t dev_addr,
    const uint8_t idx,
    uint8_t const *report_desc,
    const uint16_t desc_len
) {
    uint16_t vid, pid;
    if (!tuh_vid_pid_get(dev_addr, &vid, &pid)) {
        printf("Could not get VID/PID for device %u\n", dev_addr);
        return;
    }

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, idx);
    uint8_t *data_report_desc = malloc(desc_len);
    if (data_report_desc == NULL) {
        printf("Not enough memory to mount HID device\n");
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

    printf("[%u] HID idx: %u is mounted ift: %u (vid: %04x, pid: %04x)\n", dev_addr, idx, itf_protocol, vid, pid);
    debug_print_buffer(report_desc, desc_len);
    printf("\n");


    if (!tuh_hid_receive_report(dev_addr, idx)) {
        printf("Error: cannot request report\n");
    }
}

void tuh_hid_set_report_complete_cb(
    uint8_t dev_addr,
    uint8_t idx,
    uint8_t report_id,
    uint8_t report_type,
    uint16_t len) {
    printf("Received report complete callback for dev_addr: %u, idx: %u, report_id: %u, report_type: %u, len: %u\n",
           dev_addr, idx, report_id, report_type, len);
    should_process_cth_messages = true;
}

void tuh_hid_set_protocol_complete_cb(
    const uint8_t dev_addr,
    const uint8_t idx,
    const uint8_t protocol
) {
    printf("[%u] HID idx: %u set protocol to %u\n", dev_addr, idx, protocol);
    should_process_cth_messages = true;
}

void tuh_hid_umount_cb(
    const uint8_t dev_addr,
    const uint8_t idx
) {
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

    printf("[%u] HID idx: %u is unmounted\n", dev_addr, idx);
}


void tuh_hid_report_received_cb(
    const uint8_t dev_addr,
    const uint8_t idx,
    uint8_t const *report,
    const uint16_t len
) {
    printf("Received report from device %u, interface %u, length %u\n", dev_addr, idx, len);
    debug_print_buffer(report, len);

    if (len == 0) {
        printf("Received empty report\n");
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
        printf("Error: report data too long: %u\n", len);
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

    /*
    bool protocol_switched = false;
    if (itf_protocol == HID_ITF_PROTOCOL_MOUSE || itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
        if (tuh_hid_get_protocol(dev_addr, idx) == HID_PROTOCOL_BOOT) {
            if (!tuh_hid_set_protocol(dev_addr, idx, HID_PROTOCOL_REPORT)) {
                printf("Could not set protocol to report for device %u\n", dev_addr);
            } else {
                printf("Set protocol to report for device %u\n", dev_addr);
                protocol_switched = true;
            }
        }
    }

    if (!protocol_switched) {*/
    if (!tuh_hid_receive_report(dev_addr, idx)) {
        printf("Error: cannot request report\n");
    }
    /*}*/
}

int main() {
    watchdog_enable(5000, 1);

    // To use Pico-PIO-USB the system clock should be multiple of 12MHz.
    // USB requires a very precise clock and since PIO are working at a multiple
    // of the core frequency if it's not a multiple of 12MHz, some cycle will be longer or shorter and it will
    // create error when writing / reading usb.
    // FIXME: Can we update this for pico 2 ? like 144000
    set_sys_clock_khz(120000, true);

    sleep_ms(10);

    stdio_init_all();

    queue_init(&htc_msg_queue, sizeof(hid_to_computer_message_t), 32);
    queue_init(&cth_msg_queue, sizeof(computer_to_hid_message_t), 32);

    board_init();

    hid_mgr_init();

    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    board_init_after_tusb();
    uint64_t last_device_mounted = 0;

    while (true) {
        tud_task(); // tinyusb device task

        if (last_device_mounted) {
            uint64_t now = time_us_64();
            // Wait 500 ms before setting up usb device to wait for all device to be mounted
            if (now - last_device_mounted > 1'000'000) {
                last_device_mounted = 0;
                printf("Initializing USB device\n");
                tud_deinit(BOARD_TUD_RHPORT);

                // init device stack on configured roothub port
                const tusb_rhport_init_t rh_init = {
                    .role = TUSB_ROLE_DEVICE,
                    .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL
                };
                if (!tud_rhport_init(BOARD_TUD_RHPORT, &rh_init)) {
                    printf("Failed to initialize USB device\n");
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
                    if (data->report_data_len > 2 && data->report_data[2] == 0x48) {
                        printf("Resetting USB\n");
                        multicore_reset_core1();
                        reset_usb_boot(0, 0);
                    }
                    if (!tud_hid_ready()) {
                        printf("tud hid not ready\n");
                        continue;
                    }
                    const uint8_t interface_idx = hid_mgr_get_hid_idx(data->dev_addr, data->idx);

                    /*
                    printf("dev_addr: %u, idx: %u, sending %u\n",data->dev_addr, data->idx, data->report_data_len);
                    printf("  ");
                    debug_print_buffer(data->report_data, data->report_data_len);
                    printf("\n");*/
                    if (!tud_hid_n_report(interface_idx, data->report_id, data->report_data, data->report_data_len)) {
                        printf("tud_hid_n_report failed\n");
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
    for (uint8_t i = 0; i < hid_mgr_get_max_hid_count(); i++) {
        const hid_t *hid = hid_mgr_get(i);
        if (hid == nullptr) {
            continue;
        }
        // Only keyboard and mouse support boot protocol
        if (hid->interface_protocol == HID_ITF_PROTOCOL_NONE)
            continue;

        const uint8_t hid_protocol = tud_hid_n_get_protocol(i);
        printf("HID protocol for device %u interface %u is %u\n", hid->dev_addr, hid->interface_idx, hid_protocol);
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
    const uint16_t reqlen) {
    // TODO not Implemented
    (void) instance;
    (void) report_id;
    (void) report_type;
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
    printf("tud_hid_set_report_cb called \n");
    printf("instance: %u, report_id: %u, report_type: %u, bufsize: %u\n", instance, report_id, report_type, bufsize);
    printf("buffer: ");
    debug_print_buffer(buffer, bufsize);
    printf("\n");

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
        printf("buffer size too large\n");
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
    printf("tud_hid_set_protocol_cb called %u %u\n", instance, protocol);

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
