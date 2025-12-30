/*
 * BL_Functions.c
 *
 *  Created on: 27 Ara 2025
 *      Author: Oguzm
 */

#include "BL_Functions.h"
#include "aes.h"
#include "keys.h"
#include "mem_layout.h"
#include "tiny_printf.h"
#include "stm32f7xx_hal.h"

extern const uint8_t AES_SECRET_KEY[];

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
    memcpy(cfg, (void *)CONFIG_SECTOR_ADDR, sizeof(BootConfig_t));

    // If config is empty (0xFFFFFFFF) or invalid, set defaults
    if (cfg->magic_number != 0xDEADBEEF) {
        cfg->magic_number = 0xDEADBEEF;
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

    // CHANGE 1: Use 32-bit pointer and count
    uint32_t *data_ptr = (uint32_t *)cfg;
    int num_words = sizeof(BootConfig_t) / 4; // 32 bytes / 4 = 8 Words

    // 1. Disable Interrupts (Critical Safety)
    __disable_irq();

    HAL_FLASH_Unlock();

    // -- Step A: Erase Sector 2 --
    FLASH_EraseInitTypeDef erase_init = {0};
    erase_init.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase_init.Sector       = CONFIG_SECTOR_NUM;
    erase_init.NbSectors    = 1;

    if (HAL_FLASHEx_Erase(&erase_init, &SectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        __enable_irq(); // Re-enable on failure
        return 0;
    }

    // -- Step B: Write New Struct (Word by Word) --
    for (int i = 0; i < num_words; i++) {
        // CHANGE 2: Program 32-bit Word
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                              CONFIG_SECTOR_ADDR + (i * 4), // Offset by 4 bytes
                              data_ptr[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            __enable_irq(); // Re-enable on failure
            return 0;
        }
    }

    HAL_FLASH_Lock();

    // 2. Re-enable Interrupts
    __enable_irq();

    return 1;
}

// --- Helper: Copy One Sector to Another (No RAM Buffer) ---
static uint8_t BL_Direct_Copy(uint32_t src_addr, uint32_t dest_addr) {

    // [SAFETY] Disable interrupts to prevent Vector Table fetches during Erase
    __disable_irq();

    // 1. We MUST erase the destination first.
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    if (dest_addr == APP_ACTIVE_START_ADDR) erase.Sector = FLASH_SECTOR_5;
    else if (dest_addr == APP_DOWNLOAD_START_ADDR) erase.Sector = FLASH_SECTOR_6;
    else erase.Sector = FLASH_SECTOR_7;

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    // Erase
    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
        __enable_irq(); // [SAFETY] Re-enable on failure
        return 0; // Error
    }

    // 2. The Loop: Read -> Write directly (32-bit WORD)
    // CHANGE 1: Loop condition uses *4 instead of *8
    for (uint32_t i = 0; (i * 4) < SLOT_SIZE; i++) {

        // CHANGE 2: Read 32-bit word (uint32_t)
        // We cast the source address to a uint32_t pointer
    	uint32_t data = *(uint32_t*)(src_addr + (i * 4));

        // CHANGE 3: Write using FLASH_TYPEPROGRAM_WORD
        // Note: HAL_FLASH_Program takes 64-bit Data arg, but uses lower 32-bits for WORD type.
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest_addr + (i * 4), data) != HAL_OK) {
            __enable_irq(); // [SAFETY] Re-enable on failure
            return 0; // Error
        }
    }

    // [SAFETY] Re-enable interrupts after success
    __enable_irq();
    return 1; // Success
}

static uint8_t BL_Encrypted_Copy(uint32_t src_addr, uint32_t dest_addr) {
    struct tc_aes_key_sched_struct s;
    uint8_t buffer_enc[16]; // Read buffer (Ciphertext)
    uint8_t buffer_dec[16]; // Write buffer (Plaintext)

    // 1. Initialize AES with your secret key
    if (tc_aes128_set_decrypt_key(&s, AES_SECRET_KEY) == 0) {
        return 0; // Key init failed
    }

    // 2. Erase Destination Sector
    FLASH_EraseInitTypeDef erase;
    uint32_t error;
    // ... (Your existing erase logic here) ...
    // Define sector based on dest_addr (same as your old code)
    if (dest_addr == APP_ACTIVE_START_ADDR) erase.Sector = FLASH_SECTOR_5;
    else if (dest_addr == APP_DOWNLOAD_START_ADDR) erase.Sector = FLASH_SECTOR_6;
    else erase.Sector = FLASH_SECTOR_7;

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    HAL_FLASH_Unlock();
    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
        HAL_FLASH_Lock();
        return 0;
    }

    // 3. Copy Loop (Process 16 bytes at a time)
    for (uint32_t offset = 0; offset < SLOT_SIZE; offset += 16) {

        // A. Read 16 bytes from Source (Flash) to RAM
        memcpy(buffer_enc, (void*)(src_addr + offset), 16);

        // B. Decrypt (AES-128 ECB for simplicity, or CBC if you track IV)
        // If simply encrypting blocks:
        if (tc_aes_decrypt(buffer_dec, buffer_enc, &s) == 0) {
             HAL_FLASH_Lock(); return 0; // Decrypt failed
        }

        // C. Write 16 decrypted bytes to Destination (Flash)
        // Flash programming is usually done word-by-word (4 bytes)
        for (int i = 0; i < 4; i++) {
            uint32_t data_word;
            memcpy(&data_word, &buffer_dec[i*4], 4);

            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest_addr + offset + (i*4), data_word) != HAL_OK) {
                HAL_FLASH_Lock();
                return 0; // Write Error
            }
        }
    }

    HAL_FLASH_Lock();
    return 1; // Success
}

// --- Main Swap Function ---
void BL_Swap_NoBuffer(void) {
    HAL_FLASH_Unlock();
    printf("Starting Direct Swap (Zero-RAM Mode)...\r\n");

    // 1. Copy New App (S6) -> Scratchpad (S7)
    if (!BL_Encrypted_Copy(APP_DOWNLOAD_START_ADDR, SCRATCH_ADDR)) {
        printf("Fail: Copy S6->S7\r\n");
        HAL_FLASH_Lock(); return;
    }

    // 2. Copy Old App (S5) -> Backup (S6)
    if (!BL_Encrypted_Copy(APP_ACTIVE_START_ADDR, APP_DOWNLOAD_START_ADDR)) {
        printf("Fail: Copy S5->S6\r\n");
        HAL_FLASH_Lock(); return;
    }

    // 3. Copy New App (S7) -> Active (S5)
    if (!BL_Encrypted_Copy(SCRATCH_ADDR, APP_ACTIVE_START_ADDR)) {
        printf("Fail: Copy S7->S5\r\n");
        // CRITICAL: S5 is corrupted. Recovery needed on reboot.
        HAL_FLASH_Lock(); return;
    }

    HAL_FLASH_Lock();
    printf("Swap Complete.\r\n");
}


