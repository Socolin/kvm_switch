#include "usb_device.h"

#include "class/hid/hid.h"
#include "class/hid/hid_device.h"

#include "logger.h"
#include "hid_manager.h"
#include "kvm_switch.h"

static const uint8_t computer_id = 0;

// ╔══════════════════════════════════╗
// ║                Core              ║
// ╚══════════════════════════════════╝

void usb_device_init() {
}

void usb_device_task() {
    tud_task(); // tinyusb device task
}

void usb_device_connect_to_computer() {
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
    }
}

// ╔══════════════════════════════════╗
// ║         tinyusb callbacks        ║
// ╚══════════════════════════════════╝

void tud_mount_cb() {
    logf_debug("device mounted");

    for (uint8_t kvm_hid_idx = 0; kvm_hid_idx < hid_mgr_get_max_hid_count(); kvm_hid_idx++) {
        const hid_t *hid = hid_mgr_get(kvm_hid_idx);
        if (hid == nullptr) {
            continue;
        }
        // Only keyboard and mouse support boot protocol
        if (hid->interface_protocol == HID_ITF_PROTOCOL_NONE)
            continue;

        const uint8_t hid_protocol = tud_hid_n_get_protocol(kvm_hid_idx);
        logf_debug("HID protocol for device: %u interface: %u is: %u", hid->dev_addr, hid->interface_idx, hid_protocol);

        kvm_switch_computer_set_hid_protocol(computer_id, kvm_hid_idx, hid_protocol);
    }
}

void tud_umount_cb() {
    logf_debug("device unmounted");

    // FIXME: Reset computer state
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

    kvm_switch_computer_set_report(computer_id, instance, report_id, report_type, buffer, bufsize);
}

void tud_hid_set_protocol_cb(
    const uint8_t instance,
    const uint8_t protocol
) {
    logf_debug("instance:%u protocol:%u", instance, protocol);

    kvm_switch_computer_set_hid_protocol(computer_id, instance, protocol);
}
