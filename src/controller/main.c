#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <string.h>

#include "computer_manager.h"
#include "../shared/hid_manager.h"
#include "kvm_switch.h"
#include "../shared/logger.h"
#include "bsp/board_api.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "../shared/usb_device.h"
#include "usb_host.h"
#include "pico/bootrom.h"

#ifndef BUILD_DATE
#define BUILD_DATE "No date"
#endif

// https://pid.codes/pids/
// FIXME: Request PID when needed. Also evaluate possibility to make this configurable to allow to easily change it to
// avoid hid caching issue on windows.
#define USB_PID   0x50C0
#define USB_VID   0x1209


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
        watchdog_update();
    }
}

int main() {
    watchdog_enable(5000, 1);

    stdio_init_all();

    logger_init(LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG);

    quick_reset_button_init();

    log_info("KVM controller is starting");
    logf_info("Version: %s", BUILD_DATE);

    // To use Pico-PIO-USB the system clock should be multiple of 12MHz.
    // USB requires a very precise clock and since PIO are working at a multiple
    // of the core frequency if it's not a multiple of 12MHz, some cycle will be longer or shorter and it will
    // create error when writing / reading usb.
    // FIXME: Can we update this for pico 2 ? like 144000
    set_sys_clock_khz(120000, true);

    sleep_ms(10);

    computer_manager_init();
    computer_manager_configure_computer(1, 6, 7);

    hid_mgr_init();
    kvm_switch_init();
    usb_device_init(
        0,
        USB_VID,
        USB_PID,
        kvm_switch_computer_set_report,
        kvm_switch_computer_set_hid_protocol
    );

    log_info("Initializing board");

    board_init();

    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    board_init_after_tusb();

    log_debug("Starting main loop on core 0");

    while (true) {
        usb_device_task();
        kvm_switch_task();
        watchdog_update();
    }
}
