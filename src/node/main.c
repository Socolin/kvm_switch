#include "pico_utils.h"
#if !PICO_RP2350
#error "This targets the Pico 2 (RP2350) only"
#endif

#include <string.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "pico/multicore.h"
#include "pico/stdio.h"

#include "../shared_usb/usb_device.h"

#include "hid_manager.h"
#include "kvm_switch_node.h"
#include "logger.h"
#include "node_link_node.h"

#ifndef BUILD_DATE
#define BUILD_DATE "No date"
#endif

#define QUICK_RESET_GPIO 28

static void irq_handler(
    const uint gpio,
    const uint32_t event_mask
) {
    if (gpio == QUICK_RESET_GPIO && event_mask == GPIO_IRQ_EDGE_FALL) {
        reboot_in_bootsel();
    }
}

static void quick_reset_button_init() {
    gpio_init(QUICK_RESET_GPIO);
    gpio_set_dir(QUICK_RESET_GPIO, GPIO_IN);
    gpio_pull_up(QUICK_RESET_GPIO);

    gpio_set_irq_enabled_with_callback(QUICK_RESET_GPIO, GPIO_IRQ_EDGE_FALL, true, irq_handler);
}

static void core1_main() {
    log_debug("Starting node_link loop core 1");
    node_link_node_run();
}

int main() {
    // FIXME: Set clock to the same as controller
    set_sys_clock_khz(120'000, true);

    watchdog_enable(5000, 1);

    stdio_init_all();

    logger_init(LOG_LEVEL_DEBUG, LOG_LEVEL_INFO);

    log_info("KVM node is starting");
    // FIXME: Add commit hash
    logf_info("Version: %s", BUILD_DATE);

    quick_reset_button_init();

    node_link_node_init();
    hid_mgr_init();
    kvm_switch_node_init();
    usb_device_init(
        kvm_switch_node_computer_set_report,
        kvm_switch_node_computer_set_hid_protocol,
        kvm_switch_node_usb_device_mounted,
        kvm_switch_node_usb_device_unmounted
    );

    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    node_link_node_enqueue_init();

    while (true) {
        kvm_switch_node_task();
        usb_device_task();
        watchdog_update();
    }
    return 0;
}
