#include "hid_device_manager.h"
#if !PICO_RP2350
#error "This targets the Pico 2 (RP2350) only"
#endif

#include "bsp/board_api.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <string.h>

#include "../shared_usb/usb_device.h"

#include "computer_manager.h"
#include "hid_manager.h"
#include "kvm_switch_config.h"
#include "kvm_switch_controller.h"
#include "logger.h"
#include "node_link_ctrl.h"
#include "quick_reset_button.h"
#include "usb_host.h"

#ifndef BUILD_DATE
#define BUILD_DATE "No date"
#endif


// ╔══════════════════════════════════╗
// ║     Callback for usb_device      ║
// ╚══════════════════════════════════╝

static void on_usb_device_set_report(
    uint8_t kvm_hid_idx,
    uint8_t report_id,
    uint8_t report_type,
    uint8_t const *report_data,
    uint16_t report_data_len
);

static void on_usb_device_set_hid_protocol(
    uint8_t kvm_hid_idx,
    uint8_t hid_protocol
);

static void on_usb_device_mounted();

static void on_usb_device_unmounted();

// ╔══════════════════════════════════╗
// ║            Main Logic            ║
// ╚══════════════════════════════════╝

static void core1_main() {
    log_info("Starting USB Host on core 1");

    usb_host_init();

    log_info("USB Host ready");

    log_debug("Starting main loop on core 1");

    while (true) {
        usb_host_task();
    }
}

int main() {
    // To use Pico-PIO-USB the system clock should be multiple of 12MHz.
    // USB requires a very precise clock and since PIO are working at a multiple
    // of the core frequency if it's not a multiple of 12MHz, some cycle will be longer or shorter and it will
    // create error when writing / reading usb.
    set_sys_clock_khz(144'000, true);

    watchdog_enable(5000, 1);

    stdio_init_all();

    logger_init(LOG_LEVEL_DEBUG, LOG_LEVEL_INFO);

    quick_reset_button_init();

    log_info("KVM controller is starting");
    logf_info("Version: %s", BUILD_DATE);

    computer_manager_init();
    computer_manager_configure_computer(1, 22, 20, 21);

    hid_device_manager_init();
    hid_mgr_init();
    kvm_config_init();
    kvm_switch_controller_init();
    node_link_ctrl_init();

    usb_device_init(
        on_usb_device_set_report,
        on_usb_device_set_hid_protocol,
        on_usb_device_mounted,
        on_usb_device_unmounted
    );

    log_info("Initializing board");

    board_init();

    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    board_init_after_tusb();

    log_info("KVM Core ready");

    log_debug("Restarting KVM nodes and waiting for themn to be up");
    node_link_ctrl_restart_nodes();

    log_debug("Starting main loop on core 0");

    while (true) {
        usb_device_task();
        kvm_switch_controller_task();
        node_link_ctrl_task();
        watchdog_update();
    }
}


// ╔══════════════════════════════════╗
// ║     Callbacks implementations    ║
// ╚══════════════════════════════════╝

static void on_usb_device_set_report(
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint8_t report_type,
    uint8_t const *report_data,
    const uint16_t report_data_len
) {
    kvm_switch_controller_computer_set_report(
        LOCAL_COMPUTER_ID,
        kvm_hid_idx,
        report_id,
        report_type,
        report_data,
        report_data_len
    );
}

static void on_usb_device_set_hid_protocol(
    const uint8_t kvm_hid_idx,
    const uint8_t hid_protocol
) {
    kvm_switch_controller_computer_set_hid_protocol(
        LOCAL_COMPUTER_ID,
        kvm_hid_idx,
        hid_protocol
    );
}

static void on_usb_device_mounted(
) {
    kvm_switch_controller_enqueue_usb_device_mounted();
}

static void on_usb_device_unmounted(
) {
    kvm_switch_controller_enqueue_usb_device_unmounted();
}
