/*
 * BL_Functions.c
 *
 *  Created on: 27 Ara 2025
 *      Author: Oguzm
 */

#include "BL_Functions.h"
#include "mem_layout.h"
#include "tiny_printf.h"

/**
 * @brief  Reads bootloader configuration from flash memory.
 *
 * This function loads the persistent boot configuration structure
 * from the dedicated configuration flash sector (Sector 2).
 * If the configuration is missing or invalid (magic number mismatch),
 * default values are initialized in RAM.
 *
 * @param[out] cfg  Pointer to BootConfig_t structure to be filled.
 *
 * @retval 1  Configuration was empty or invalid and defaults were applied.
 * @retval 0  Configuration was valid and read successfully.
 */
uint8_t BL_ReadConfig(BootConfig_t *cfg) {
	uint8_t config_was_empty = 0;
    // Direct memory read
    memcpy(cfg, CONFIG_FLASH_PTR, sizeof(BootConfig_t));

    // If config is empty (0xFFFFFFFF) or invalid, set defaults
    if (cfg->magic_number != 0xCAFEFEED) {
        cfg->magic_number = 0xCAFEFEED;
        cfg->system_status = STATE_NORMAL;
        cfg->boot_failure_count = 0;
        cfg->active_slot = 1;
        cfg->current_version = 0;

        config_was_empty = 1;
    }
    return config_was_empty;
}

/**
 * @brief  Writes bootloader configuration to flash memory.
 *
 * This function erases the dedicated configuration sector (Sector 2)
 * and programs the provided BootConfig_t structure into flash.
 * Flash programming is performed using 64-bit double-word writes.
 *
 * @warning This function performs a flash erase operation.
 *          It must NOT be interrupted or executed from flash
 *          regions being erased.
 *
 * @param[in] cfg  Pointer to the configuration structure to be written.
 *
 * @retval 1  Configuration written successfully.
 * @retval 0  Flash erase or programming failed.
 */
uint8_t BL_WriteConfig(BootConfig_t *cfg) {

    uint32_t SectorError;
    uint64_t *data_ptr = (uint64_t *)cfg;
    int num_double_words = sizeof(BootConfig_t) / 8;

    HAL_FLASH_Unlock();
    // -- Step A: Erase Sector 2 --
    FLASH_EraseInitTypeDef erase_init =
	{
		.TypeErase = FLASH_TYPEERASE_SECTORS,
		.VoltageRange = FLASH_VOLTAGE_RANGE_3,
		.Sector = CONFIG_SECTOR_ADDR,
		.NbSectors = 1
	};

    if (HAL_FLASHEx_Erase(&erase_init, &SectorError) != HAL_OK) {
        // Handle Error (Blink LED?)
        HAL_FLASH_Lock();
        return;
    }

    // -- Step B: Write New Struct --
    for (int i = 0; i < num_double_words + 1; i++) {
        // We write 64 bits (Double Word) at a time
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                              CONFIG_SECTOR_ADDR + (i * 8),
                              data_ptr[i]) != HAL_OK) {
            // Write Error
            break;
        }
    }

    HAL_FLASH_Lock();
    return 1;
}

// --- Helper: Copy One Sector to Another (No RAM Buffer) ---
static uint8_t BL_Direct_Copy(uint32_t src_addr, uint32_t dest_addr) {

    // 1. We MUST erase the destination first.
    // You cannot write to Flash without erasing it.
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    // Determine which sector number corresponds to the address
    // (Simplification based on your layout)
    if (dest_addr == APP_ACTIVE_START_ADDR) erase.Sector = FLASH_SECTOR_5;
    else if (dest_addr == APP_DOWNLOAD_START_ADDR) erase.Sector = FLASH_SECTOR_6;
    else erase.Sector = FLASH_SECTOR_7;

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    // Blink LED to show "Erasing..."
    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) return 0; // Error

    // 2. The Loop: Read -> Write directly
    for (uint32_t i = 0; i < SLOT_SIZE; i += 8) {

        // A. Read 64-bit word directly from Source Flash
        uint64_t data = *(uint64_t*)(src_addr + i);

        // B. Write 64-bit word directly to Destination Flash
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, dest_addr + i, data) != HAL_OK) {
            return 0; // Error
        }
    }
    return 1; // Success
}

// --- Main Swap Function ---
void BL_Swap_NoBuffer(void) {
    HAL_FLASH_Unlock();
    printf("Starting Direct Swap (Zero-RAM Mode)...\r\n");

    // 1. Copy New App (S6) -> Scratchpad (S7)
    if (!BL_Direct_Copy(APP_DOWNLOAD_START_ADDR, SCRATCH_ADDR)) {
        printf("Fail: Copy S6->S7\r\n");
        HAL_FLASH_Lock(); return;
    }

    // 2. Copy Old App (S5) -> Backup (S6)
    if (!BL_Direct_Copy(APP_ACTIVE_START_ADDR, APP_DOWNLOAD_START_ADDR)) {
        printf("Fail: Copy S5->S6\r\n");
        HAL_FLASH_Lock(); return;
    }

    // 3. Copy New App (S7) -> Active (S5)
    if (!BL_Direct_Copy(SCRATCH_ADDR, APP_ACTIVE_START_ADDR)) {
        printf("Fail: Copy S7->S5\r\n");
        // CRITICAL: S5 is corrupted. Recovery needed on reboot.
        HAL_FLASH_Lock(); return;
    }

    HAL_FLASH_Lock();
    printf("Swap Complete.\r\n");
}


