#if !PICO_RP2350
#error "This targets the Pico 2 (RP2350) only"
#endif

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <string.h>

#include "computer_manager.h"
#include "node_link_ctrl.h"
#include "../shared/hid_manager.h"
#include "kvm_switch_controller.h"
#include "../shared/logger.h"
#include "bsp/board_api.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "../shared_usb/usb_device.h"
#include "usb_host.h"
#include "pico/bootrom.h"

#ifndef BUILD_DATE
#define BUILD_DATE "No date"
#endif

#define QUICK_RESET_GPIO 28

static void irq_handler(
    const uint gpio,
    const uint32_t event_mask
) {
    if (gpio == QUICK_RESET_GPIO && event_mask == GPIO_IRQ_EDGE_FALL) {
        log_critical("Resetting PICO in BOOTSEL");
        multicore_reset_core1();
        reset_usb_boot(0, 0);
    }
}

static void quick_reset_button_init() {
    gpio_init(QUICK_RESET_GPIO);
    gpio_set_dir(QUICK_RESET_GPIO, GPIO_IN);
    gpio_pull_up(QUICK_RESET_GPIO);

    gpio_set_irq_enabled_with_callback(QUICK_RESET_GPIO, GPIO_IRQ_EDGE_FALL, true, irq_handler);
}

static void core1_main() {
    log_info("Starting USB Host on core 1");

    usb_host_init();

    log_info("USB Host ready");

    log_debug("Starting main loop on core 1");

    while (true) {
        usb_host_task();
    }
}

#define RUN_NODES_GPIO 22
static void restart_kvm_nodes() {
    gpio_init(RUN_NODES_GPIO);
    gpio_set_dir(RUN_NODES_GPIO, GPIO_OUT);
    sleep_us(10);
    gpio_set_dir(RUN_NODES_GPIO, GPIO_IN);
    sleep_ms(50);
}

static void on_usb_device_set_report(
    const uint8_t kvm_hid_idx,
    const uint8_t report_id,
    const uint8_t report_type,
    uint8_t const *report_data,
    const uint16_t report_data_len
) {
    kvm_switch_controller_computer_set_report(0, kvm_hid_idx, report_id, report_type, report_data, report_data_len);
}

static void on_usb_device_set_hid_protocol(
    const uint8_t kvm_hid_idx,
    const uint8_t hid_protocol
) {
    kvm_switch_controller_computer_set_hid_protocol(0, kvm_hid_idx, hid_protocol);
}

int main() {
    // To use Pico-PIO-USB the system clock should be multiple of 12MHz.
    // USB requires a very precise clock and since PIO are working at a multiple
    // of the core frequency if it's not a multiple of 12MHz, some cycle will be longer or shorter and it will
    // create error when writing / reading usb.
    // FIXME: Can we update this for pico 2 ? like 144000
    set_sys_clock_khz(120'000, true);

    watchdog_enable(5000, 1);

    stdio_init_all();

    logger_init(LOG_LEVEL_DEBUG, LOG_LEVEL_INFO);

    quick_reset_button_init();
    restart_kvm_nodes();

    log_info("KVM controller is starting");
    logf_info("Version: %s", BUILD_DATE);
    sleep_ms(10);

    computer_manager_init();
    computer_manager_configure_computer(1, 6, 7, 8);

    hid_mgr_init();
    kvm_switch_controller_init();
    node_link_ctrl_init();

    usb_device_init(
        on_usb_device_set_report,
        on_usb_device_set_hid_protocol
    );

    log_info("Initializing board");

    board_init();

    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    board_init_after_tusb();

    log_debug("Starting main loop on core 0");

    while (true) {
        usb_device_task();
        kvm_switch_controller_task();
        node_link_ctrl_task();
        watchdog_update();
    }
}
