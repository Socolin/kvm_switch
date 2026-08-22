#include <stdio.h>
#include <string.h>

#include "extension_node.h"
#include "kvm_switch_node.h"
#include "../shared/logger.h"
#include "../shared_usb/usb_device.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pico/stdio.h"
#include "pico/time.h"

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
    log_debug("Starting extension_node on core 1");
    extension_node_init();
    log_debug("Starting extension_node loop core 1");
    extension_node_run();
}

int main() {
    watchdog_enable(5000, 1);

    stdio_init_all();

    logger_init(LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG);

    log_info("KVM node is starting");
    logf_info("Version: %s", BUILD_DATE);

    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    quick_reset_button_init();
    extension_node_init();
    kvm_switch_node_init();


    // FIXME: maybe this need to be done in kvm_switch_node
    usb_device_init(
        USB_VID,
        USB_PID,
        kvm_switch_node_computer_set_report,
        kvm_switch_node_computer_set_hid_protocol
    );

    logger_init(LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG);

    log_info("KVM node starting");
    logf_info("Version: %s", BUILD_DATE);

    while (true) {
        kvm_switch_node_task();
        usb_device_task();
        watchdog_update();
    }
    return 0;
}
