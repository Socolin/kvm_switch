#include "quick_reset_button.h"

#include "gpio_utils.h"
#include "pico_utils.h"
#include "hardware/gpio.h"

#define QUICK_RESET_GPIO 28

static void button_pressed(
    [[maybe_unused]] uint gpio,
    [[maybe_unused]] uint32_t event_mask,
    [[maybe_unused]] void *user_data
) {
    reboot_in_bootsel();
}

void quick_reset_button_init() {
    gpio_init(QUICK_RESET_GPIO);
    gpio_set_dir(QUICK_RESET_GPIO, GPIO_IN);
    gpio_pull_up(QUICK_RESET_GPIO);

    gpio_util_set_handler(QUICK_RESET_GPIO, GPIO_IRQ_EDGE_FALL, button_pressed, nullptr);
}
