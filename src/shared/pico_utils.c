#include "pico_utils.h"


#include "logger.h"
#include "pico/bootrom.h"
#include "pico/multicore.h"

void reboot_in_bootsel() {
    log_critical("Resetting PICO in BOOTSEL");
    multicore_reset_core1();
    reset_usb_boot(0, 0);
}
