#include "config_persistence.h"

#include "logger.h"
#include "hardware/flash.h"
#include "pico/flash.h"

// We cannot use the last sector, because it's used as a workaround to a problem with the bootrom RP2350-E10
// From RP2350 Datasheet:
// > This workaround means that the last block of flash is erased when downloading such a UF2, which could
// > overwrite user data.
// Moreover to be futureproof, the bluetooth stack may use the last 2 sectors, and is also applying the workaround
// So let's keep the last 3 sectors free
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - (FLASH_SECTOR_SIZE * 4))

typedef struct flash_param {
    const uint8_t *data;
    const size_t size;
} write_flash_param;

static void write_data_to_flash(void *params) {
    const write_flash_param *write_params = (write_flash_param *) params;

    // We can only erase flash in sector-sized chunks (4096 bytes / FLASH_SECTOR_SIZE), this set all the bytes to 1
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    // Then we can write to the flash by PAGE increment (256 bytes / FLASH_PAGE_SIZE);)
    for (size_t data_offset = 0; data_offset < write_params->size; data_offset += FLASH_PAGE_SIZE)
        flash_range_program(FLASH_TARGET_OFFSET + data_offset, write_params->data + data_offset, FLASH_PAGE_SIZE);

    const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    bool mismatch = false;
    for (uint i = 0; i < write_params->size; ++i) {
        if (write_params->data[i] != flash_target_contents[i])
            mismatch = true;
    }
    if (mismatch) {
        log_critical("Error while writing data to flash: Flash contents mismatch.");
    }
}

void config_persistence_save(
    const void *config_data,
    const size_t config_data_len
) {
    write_flash_param param = {
        .data = config_data,
        .size = config_data_len
    };

    const int rc = flash_safe_execute(write_data_to_flash, &param, UINT32_MAX);
    if (rc != PICO_OK) {
        logf_critical("Failed to save config to flash");
        return;
    }

    logf_info("Config saved to flash");
}

void config_persistence_read(
    const void *config_data,
    const size_t config_data_len
) {
    const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    memcpy((void *) config_data, flash_target_contents, config_data_len);
}
