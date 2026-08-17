#include "usb_host.h"

#include <string.h>

#include "pico/util/queue.h"
#include "host/usbh.h"
#include "pio_usb.h"
#include "pio_usb_configuration.h"

#include "logger.h"
#include "hid_manager.h"
#include "kvm_switch.h"

static queue_t hid_action_queue = {};

void usb_host_init() {
    queue_init(&hid_action_queue, sizeof(hid_action_t), 32);

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
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    tuh_init(BOARD_TUH_RHPORT);

    pio_usb_host_add_port(18, PIO_USB_PINOUT_DPDM);
}

// When an action is already in progress, we cannot process any other actions
static bool should_process_actions = true;

static void usb_host_process_action() {
    if (!should_process_actions)
        return;

    hid_action_t hid_action;
    if (!queue_try_peek(&hid_action_queue, &hid_action))
        return; // Nothing to do

    switch (hid_action.opcode) {
        case HID_SET_REPORT: {
            hid_action_set_report_t *data = (hid_action_set_report_t *) hid_action.data;
            should_process_actions = false;
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
        case HID_SET_PROTOCOL: {
            const hid_action_set_protocol_t *data = (hid_action_set_protocol_t *) hid_action.data;
            if (tuh_hid_get_protocol(data->dev_addr, data->itf_idx) == data->hid_protocol)
                break;
            should_process_actions = false;
            if (!tuh_hid_set_protocol(data->dev_addr, data->itf_idx, data->hid_protocol)) {
                logf_error("Failed to set HID protocol for device %u interface %u to %u", data->dev_addr,
                           data->itf_idx, data->hid_protocol);
                return;
            }
            break;
        }
        default:
            logf_critical("Unknown computer to hid opcode %u", hid_action.opcode);
            break;
    }

    queue_try_remove(&hid_action_queue, nullptr);
}

void usb_host_task() {
    tuh_task();
    usb_host_process_action();
}

// ╔══════════════════════════════════╗
// ║             HID actions          ║
// ╚══════════════════════════════════╝

static bool usb_host_enqueue_hid_action(
    const hid_action_opcode_t opcode,
    const void *data,
    const size_t data_len

) {
    hid_action_t action = {
        .opcode = opcode,
        .data_len = data_len,
    };
    memcpy(action.data, data, data_len);
    return queue_try_add(&hid_action_queue, &action);
}

bool usb_host_enqueue_set_protocol(
    const uint8_t kvm_hid_idx,
    const uint8_t hid_protocol
) {
    const hid_t *hid = hid_mgr_get(kvm_hid_idx);
    if (hid == nullptr)
        return false;

    const hid_action_set_protocol_t action_data = {
        .dev_addr = hid->dev_addr,
        .itf_idx = hid->interface_idx,
        .hid_protocol = hid_protocol,
    };

    return usb_host_enqueue_hid_action(HID_SET_PROTOCOL, &action_data, sizeof(action_data));
}

bool usb_host_enqueue_set_report(
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint8_t report_type,
    uint8_t const *report_data,
    const uint16_t report_data_len
) {
    const hid_t *hid = hid_mgr_get(kvm_hid_idx);
    if (hid == nullptr)
        return false;

    hid_action_set_report_t action_data = {
        .dev_addr = hid->dev_addr,
        .itf_idx = hid->interface_idx,
        .report_type = report_type,
        .report_id = report_id,
        .buffer_len = report_data_len,
    };
    if (report_data_len > sizeof(action_data.buffer)) {
        log_error("report_data_len is too large");
        return false;
    }
    memcpy(action_data.buffer, report_data, report_data_len);

    return usb_host_enqueue_hid_action(HID_SET_REPORT, &action_data, sizeof(action_data));
}

// ╔══════════════════════════════════╗
// ║         tinyusb callbacks        ║
// ╚══════════════════════════════════╝

void tuh_mount_cb(
    const uint8_t dev_addr
) {
    logf_debug("dev_addr: %u", dev_addr);

    kvm_switch_enqueue_device_mount(dev_addr);

    logf_info("Device %u is mounted", dev_addr);
}

void tuh_umount_cb(
    const uint8_t dev_addr
) {
    logf_debug("dev_addr: %u", dev_addr);

    kvm_switch_enqueue_device_umount(dev_addr);

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

    const uint8_t itf_protocol = tuh_hid_interface_protocol(dev_addr, idx);
    kvm_switch_enqueue_hid_mount(dev_addr, idx, itf_protocol, pid, vid, report_desc, desc_len);

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
    should_process_actions = true;
}

void tuh_hid_set_protocol_complete_cb(
    const uint8_t dev_addr,
    const uint8_t idx,
    const uint8_t protocol
) {
    logf_debug("dev_addr: %u, idx: %u, protocol: %u", dev_addr, idx, protocol);
    should_process_actions = true;
}

void tuh_hid_umount_cb(
    const uint8_t dev_addr,
    const uint8_t idx
) {
    logf_debug("dev_addr: %u, idx: %u", dev_addr, idx);

    kvm_switch_enqueue_hid_umount(dev_addr, idx);

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

    kvm_switch_enqueue_report(dev_addr, idx, itf_protocol, hid_protocol, report, len);

    if (!tuh_hid_receive_report(dev_addr, idx)) {
        log_error("tuh_hid_receive_report failed");
    }
}
