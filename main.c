#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <string.h>

#include "hid_manager.h"
#include "kvm_switch.h"
#include "logger.h"
#include "bsp/board_api.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "usb_device.h"
#include "usb_host.h"

#ifndef BUILD_DATE
#define BUILD_DATE "No date"
#endif

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

    log_info("KVM is starting");
    logf_info("Version: %s", BUILD_DATE);

    // To use Pico-PIO-USB the system clock should be multiple of 12MHz.
    // USB requires a very precise clock and since PIO are working at a multiple
    // of the core frequency if it's not a multiple of 12MHz, some cycle will be longer or shorter and it will
    // create error when writing / reading usb.
    // FIXME: Can we update this for pico 2 ? like 144000
    set_sys_clock_khz(120000, true);

    sleep_ms(10);

    hid_mgr_init();
    kvm_switch_init();
    usb_device_init();

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
