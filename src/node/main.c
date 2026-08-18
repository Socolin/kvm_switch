#include <stdio.h>
#include <string.h>

#include "../shared/logger.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pico/stdio.h"
#include "pico/time.h"

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

int main() {
    watchdog_enable(5000, 1);

    stdio_init_all();

    quick_reset_button_init();
    logger_init(LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG);

    log_info("KVM node starting");
    logf_info("Version: %s", BUILD_DATE);

    int previous_get = -1;
    while (true) {
        sleep_ms(100);

        bool get = gpio_get(28);
        if (get != previous_get)
            printf("GPIO 28: %d\n", get);
        previous_get = get;
        watchdog_update();
    }
    return 0;
}
