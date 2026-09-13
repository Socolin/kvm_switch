#include "quick_reset_button.h"
#if !PICO_RP2350
#error "This targets the Pico 2 (RP2350) only"
#endif

#include <string.h>

#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "pico/multicore.h"
#include "pico/stdio.h"

#include "../shared_usb/usb_device.h"

#include "hid_manager.h"
#include "kvm_switch_node.h"
#include "logger.h"
#include "node_link_node.h"
#include "version.h"
#include "git_version.h"

static void core1_main() {
    log_debug("Starting node_link loop core 1");
    node_link_node_run();
}

int main() {
    set_sys_clock_khz(144'000, true);

    watchdog_enable(5000, 1);

    stdio_init_all();

    logger_init(LOG_LEVEL_DEBUG, LOG_LEVEL_INFO);

    log_info("KVM node is starting");
    logf_info("Version: %s", GIT_HASH);
    logf_info("Built at : %s", BUILD_DATE);

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
