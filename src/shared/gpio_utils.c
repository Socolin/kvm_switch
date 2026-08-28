#include "gpio_utils.h"

#include "hardware/gpio.h"

typedef struct {
    bool initialized;
    gpio_handler_t handlers[NUM_BANK0_GPIOS];
    const void* handle_user_data[NUM_BANK0_GPIOS];
} gpio_util_t;

static gpio_util_t gpio_util = {};

static void gpio_irq_handler(
    const uint gpio,
    const uint32_t event_mask
) {
    if (gpio < NUM_BANK0_GPIOS && gpio_util.handlers[gpio]) {
        gpio_util.handlers[gpio](gpio, event_mask, gpio_util.handle_user_data[gpio]);
    }
}

static void gpio_util_enable_irq() {
    gpio_set_irq_callback(gpio_irq_handler);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void gpio_util_set_handler(
    const uint gpio,
    const uint32_t event_mask,
    const gpio_handler_t handler,
    const void *user_data
) {
    if (!gpio_util.initialized)
        gpio_util_enable_irq();
    gpio_util.handlers[gpio] = handler;
    gpio_util.handle_user_data[gpio] = user_data;

    gpio_set_irq_enabled(gpio, event_mask, true);
}

void gpio_util_clear_handler(
    const uint gpio
) {
    gpio_set_irq_enabled(gpio, 0, false);
    gpio_util.handlers[gpio] = nullptr;
    gpio_util.handle_user_data[gpio] = nullptr;
}
