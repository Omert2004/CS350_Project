/**
 * @file    BL_Functions.c
 * @brief   Implementation of core Bootloader logic.
 * @details Handles configuration management, AES encryption/decryption (CBC/ECB),
 * and the orchestration of Firmware Updates and Rollbacks.
 * @version 1.7
 */

#include "BL_Functions.h"
#include "aes.h"
#include "keys.h"
#include "mem_layout.h"
#include "tiny_printf.h"
#include "stm32f7xx_hal.h"
#include "string.h"
#include "Cryptology_Control.h"
#include "firmware_footer.h"

extern const uint8_t AES_SECRET_KEY[];

// ============================================================================
// CONFIGURATION FUNCTIONS
// ============================================================================

/**
 * @brief  Reads the Bootloader Configuration from Flash.
 * @note   If the Magic Number is invalid, initializes the config with defaults.
 * @param  cfg Pointer to the BootConfig_t structure to populate.
 * @retval 0 if config was valid, 1 if defaults were loaded.
 */
uint8_t BL_ReadConfig(BootConfig_t *cfg) {
    memcpy(cfg, (void *)CONFIG_SECTOR_ADDR, sizeof(BootConfig_t));
    if (cfg->magic_number != 0xDEADBEEF) {
        // Set Defaults
        cfg->magic_number = 0xDEADBEEF;
        cfg->system_status = STATE_NORMAL;
        cfg->current_version = 0;
        return 1; // Config was empty/invalid
    }
    return 0; // Config valid
}

/**
 * @brief  Writes the Bootloader Configuration to Flash.
 * @note   Erases the Config Sector (Sector 2) before writing.
 * @param  cfg Pointer to the BootConfig_t structure to write.
 * @retval 1 on success, 0 on failure.
 */
uint8_t BL_WriteConfig(BootConfig_t *cfg) {
    uint32_t SectorError;
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase_init = {0};
    erase_init.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase_init.Sector       = CONFIG_SECTOR_NUM;
    erase_init.NbSectors    = 1;

    if (HAL_FLASHEx_Erase(&erase_init, &SectorError) != HAL_OK) {
        HAL_FLASH_Lock(); return 0;
    }

    uint32_t *data_ptr = (uint32_t *)cfg;
    int num_words = sizeof(BootConfig_t) / 4;

    for (int i = 0; i < num_words; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, CONFIG_SECTOR_ADDR + (i*4), data_ptr[i]) != HAL_OK) {
            HAL_FLASH_Lock(); return 0;
        }
    }
    HAL_FLASH_Lock(); return 1;
}

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

/**
 * @brief  Raw copy of data between Flash sectors.
 * @note   Automatically selects and erases the destination sector.
 * @param  src_addr  Source address in Flash.
 * @param  dest_addr Destination address in Flash.
 * @param  size      Number of bytes to copy.
 * @retval 1 on success, 0 on failure.
 */
static uint8_t BL_Raw_Copy(uint32_t src_addr, uint32_t dest_addr, uint32_t size){
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    // Select Sector based on Address
    if (dest_addr == APP_ACTIVE_START_ADDR) erase.Sector = FLASH_SECTOR_5;
    else if (dest_addr == APP_DOWNLOAD_START_ADDR) erase.Sector = FLASH_SECTOR_6;
    else erase.Sector = FLASH_SECTOR_7;

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    printf("  [DEBUG] Erasing Sector %d... ", (int)erase.Sector);
    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
        printf("FAILED! HAL Error: 0x%x\r\n", (unsigned int)HAL_FLASH_GetError());
        HAL_FLASH_Lock(); return 0;
    }
    printf("OK\r\n");

    printf("  [DEBUG] Writing %d bytes to 0x%x... ", (int)size, (unsigned int)dest_addr);
    for (uint32_t i = 0; (i * 4) < size; i++) {
        uint32_t data = *(uint32_t*)(src_addr + (i * 4));
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest_addr + (i * 4), data) != HAL_OK) {
            printf("FAILED at offset %d!\r\n", (int)(i*4));
            HAL_FLASH_Lock(); return 0;
        }
    }
    printf("OK\r\n");
    HAL_FLASH_Lock(); return 1;
}

// ============================================================================
// CRYPTOGRAPHIC OPERATIONS
// ============================================================================

/**
 * @brief  Decrypts a new update image using AES-CBC.
 * @details Reads encrypted data from S6, decrypts it, and writes plaintext to S7.
 * @param  src_slot_addr Start address of the encrypted image.
 * @param  dest_addr     Destination address (Scratchpad).
 * @param  payload_size  Total size of IV + Ciphertext.
 * @retval 1 on success, 0 on failure.
 */
static uint8_t BL_Decrypt_Update_Image(uint32_t src_slot_addr, uint32_t dest_addr, uint32_t payload_size) {
    struct tc_aes_key_sched_struct s;
    uint8_t buffer_enc[16], buffer_dec[16], current_iv[16];
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    if (tc_aes128_set_decrypt_key(&s, AES_SECRET_KEY) == 0) return 0;

    memcpy(current_iv, (void*)src_slot_addr, 16); // Read IV

    HAL_FLASH_Unlock();
    erase.Sector = FLASH_SECTOR_7;
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
        HAL_FLASH_Lock(); return 0;
    }

    uint32_t encrypted_data_size = payload_size - 16;
    uint32_t data_start_offset = 16;

    for (uint32_t i = 0; i < encrypted_data_size; i += 16) {
        memcpy(buffer_enc, (void*)(src_slot_addr + data_start_offset + i), 16);
        tc_aes_decrypt(buffer_dec, buffer_enc, &s);
        for(int j=0; j<16; j++) buffer_dec[j] ^= current_iv[j]; // CBC XOR
        memcpy(current_iv, buffer_enc, 16);

        for (int k = 0; k < 4; k++) {
            uint32_t data_word;
            memcpy(&data_word, &buffer_dec[k*4], 4);
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest_addr + i + (k*4), data_word) != HAL_OK) {
                HAL_FLASH_Lock(); return 0;
            }
        }
    }
    HAL_FLASH_Lock(); return 1;
}

/**
 * @brief  Encrypts the active application using AES-ECB for backup.
 * @details Reads plaintext from S5, encrypts it, and writes to S6.
 * @param  src_addr  Source address (Active App).
 * @param  dest_addr Destination address (Backup Slot).
 * @retval 1 on success, 0 on failure.
 */
static uint8_t BL_Encrypt_Backup(uint32_t src_addr, uint32_t dest_addr) {
    struct tc_aes_key_sched_struct s;
    uint8_t buffer_plain[16];
    uint8_t buffer_enc[16];
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    if (tc_aes128_set_encrypt_key(&s, AES_SECRET_KEY) == 0) {
        printf("Error: AES Key Init Failed.\r\n"); return 0;
    }

    __disable_irq();
    HAL_FLASH_Unlock();

    erase.Sector = FLASH_SECTOR_6; // Backup Sector
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    printf("  [DEBUG] Erasing Backup Sector (S6)... ");
    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
        printf("FAILED! HAL Err: 0x%x\r\n", (unsigned int)HAL_FLASH_GetError());
        HAL_FLASH_Lock(); __enable_irq(); return 0;
    }
    printf("OK\r\n");

    printf("  [DEBUG] Encrypting & Backing up %d bytes... \r\n", SLOT_SIZE);

    for (uint32_t offset = 0; offset < SLOT_SIZE; offset += 16) {
        memcpy(buffer_plain, (void*)(src_addr + offset), 16);

        if (tc_aes_encrypt(buffer_enc, buffer_plain, &s) == 0) {
             HAL_FLASH_Lock(); __enable_irq(); return 0;
        }

        for (int i = 0; i < 4; i++) {
            uint32_t data_word;
            memcpy(&data_word, &buffer_enc[i*4], 4);
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest_addr + offset + (i*4), data_word) != HAL_OK) {
                printf("Backup Write Failed at offset 0x%x\r\n", (unsigned int)offset);
                HAL_FLASH_Lock(); __enable_irq(); return 0;
            }
        }
    }

    HAL_FLASH_Lock(); __enable_irq();
    printf("  Backup Complete.\r\n");
    return 1;
}

/**
 * @brief  Decrypts a backup image using AES-ECB for rollback.
 * @details Reads encrypted backup from S6, decrypts it, and writes to S7.
 * @param  src_addr  Source address (Backup Slot).
 * @param  dest_addr Destination address (Scratchpad).
 * @retval 1 on success, 0 on failure.
 */
static uint8_t BL_Decrypt_Backup_Image(uint32_t src_addr, uint32_t dest_addr) {
    struct tc_aes_key_sched_struct s;
    uint8_t buffer_enc[16];
    uint8_t buffer_dec[16];
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    if (tc_aes128_set_decrypt_key(&s, AES_SECRET_KEY) == 0) return 0;

    __disable_irq();
    HAL_FLASH_Unlock();

    erase.Sector = FLASH_SECTOR_7; // Scratch
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
        HAL_FLASH_Lock(); __enable_irq(); return 0;
    }

    for (uint32_t offset = 0; offset < SLOT_SIZE; offset += 16) {
        memcpy(buffer_enc, (void*)(src_addr + offset), 16);

        if (tc_aes_decrypt(buffer_dec, buffer_enc, &s) == 0) {
             HAL_FLASH_Lock(); __enable_irq(); return 0;
        }

        for (int k = 0; k < 4; k++) {
            uint32_t data_word;
            memcpy(&data_word, &buffer_dec[k*4], 4);
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest_addr + offset + (k*4), data_word) != HAL_OK) {
                HAL_FLASH_Lock(); __enable_irq(); return 0;
            }
        }
    }
    HAL_FLASH_Lock(); __enable_irq(); return 1;
}

// ============================================================================
// STATE MACHINE FUNCTIONS
// ============================================================================

/**
 * @brief  Executes the Firmware Update Process.
 * @details Steps:
 * 1. Verify Signature of image in Download Slot (S6).
 * 2. Decrypt Image (S6) -> Scratchpad (S7).
 * 3. Backup Active App (S5) -> Download Slot (S6) (Encrypted).
 * 4. Install New App (S7) -> Active Slot (S5).
 * 5. Reset System.
 */
void BL_Swap_NoBuffer(void) {
    BootConfig_t cfg;
    BL_ReadConfig(&cfg);

    uint32_t footer_addr = Find_Footer_Address(APP_DOWNLOAD_START_ADDR, SLOT_SIZE);
    if (footer_addr == 0) {
        printf("Error: No Footer found in S6.\r\n");
        cfg.system_status = STATE_NORMAL;
        BL_WriteConfig(&cfg);
        return;
    }

    fw_footer_t footer;
    memcpy(&footer, (void*)footer_addr, sizeof(fw_footer_t));

    printf("[BL] Verifying Signature... ");
    FW_Status_t status = Firmware_Is_Valid(APP_DOWNLOAD_START_ADDR, SLOT_SIZE);

	if (status != BL_OK) {
		printf("FAIL! Error Code: %d\r\n", status);

		//Erase the Download Slot since the Firmware is not valid.
		FLASH_EraseInitTypeDef erase = {0};
		uint32_t error;
		HAL_FLASH_Unlock();
		erase.Sector = FLASH_SECTOR_6;
		erase.TypeErase = FLASH_TYPEERASE_SECTORS;
		erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
		erase.NbSectors = 1;
		HAL_FLASHEx_Erase(&erase, &error);
		HAL_FLASH_Lock();

		if (status == BL_ERR_SIG_FAIL) printf("Reason: ECDSA Signature Mismatch.\r\n");
		if (status == BL_ERR_FOOTER_NOT_FOUND) printf("Reason: Footer Missing.\r\n");

		cfg.system_status = STATE_NORMAL;
		BL_WriteConfig(&cfg);
		return;
	}
    printf("OK!\r\n");
    printf("[BL] Valid Update! Ver: %d, Payload: %d\r\n", (int)footer.version, (int)footer.size);

    printf("[1/3] Decrypting S6 -> S7...\r\n");
    if (!BL_Decrypt_Update_Image(APP_DOWNLOAD_START_ADDR, SCRATCH_ADDR, footer.size)) {
        printf("Error: Decryption Failed.\r\n"); return;
    }

    printf("[2/3] Backing up S5 -> S6...\r\n");
    if (!BL_Encrypt_Backup(APP_ACTIVE_START_ADDR, APP_DOWNLOAD_START_ADDR)) {
        printf("Error: Backup Failed.\r\n"); return;
    }

    printf("[3/3] Installing S7 -> S5...\r\n");
    if (!BL_Raw_Copy(SCRATCH_ADDR, APP_ACTIVE_START_ADDR, SLOT_SIZE)) {
        printf("Error: Installation Failed.\r\n"); return;
    }

    printf("[BL] Update Successful! Setting State to NORMAL.\r\n");
    cfg.system_status = STATE_NORMAL;
    cfg.current_version = footer.version;
    BL_WriteConfig(&cfg);

    printf("Swap Complete. Resetting...\r\n");
    HAL_NVIC_SystemReset();
}

/**
 * @brief  Executes the Rollback/Swap Process.
 * @details Steps:
 * 1. Decrypt Backup (S6) -> Scratchpad (S7).
 * 2. Backup Active App (S5) -> Download Slot (S6) (Encrypted).
 * 3. Install Old App (S7) -> Active Slot (S5).
 * 4. Reset System.
 */
uint8_t BL_Rollback(void) {
    BootConfig_t cfg;
    BL_ReadConfig(&cfg);
    uint8_t error_code=BL_OK;

    printf("\r\n[BL] Starting Rollback/Toggle...\r\n");

    HAL_FLASH_Unlock();

    printf("[1/3] Decrypting Backup (S6 -> S7)...\r\n");
    if (!BL_Decrypt_Backup_Image(APP_DOWNLOAD_START_ADDR, SCRATCH_ADDR)) {
        printf("Error: Rollback Decryption Failed.\r\n");
        HAL_FLASH_Lock();
        return 1;
    }

    uint32_t *pDecryptedData = (uint32_t *)SCRATCH_ADDR; // Start of S7 (0x080C0000)
	uint32_t stackPointer = pDecryptedData[0];
	uint32_t resetVector  = pDecryptedData[1];

	// Check 1: Stack Pointer usually points to RAM (0x20xxxxxx)
	// Check 2: Reset Vector must be within Flash range (approx 0x08040000 to 0x08080000)

	// Simple check: Is the Reset Vector pointing to Flash?
	if ((resetVector & 0xFF000000) != 0x08000000)
	{
		printf("[ERROR] The Backup in S6 is Empty or Invalid!\r\n");
		printf("[ERROR] Reset Vector: 0x%08X. Aborting Swap to protect Active App.\r\n", (unsigned int)resetVector);

		// Return to NORMAL state without erasing S5
		BootConfig_t cfg;
		BL_ReadConfig(&cfg);
		cfg.system_status = STATE_NORMAL;
		BL_WriteConfig(&cfg);
		return 2;
	}

    printf("[2/3] Backing up Current App (S5 -> S6)...\r\n");
    if (!BL_Encrypt_Backup(APP_ACTIVE_START_ADDR, APP_DOWNLOAD_START_ADDR)) {
        printf("Error: Backup Failed.\r\n");
        HAL_FLASH_Lock();
        return 3;
    }

    printf("[3/3] Restoring Old App (S7 -> S5)...\r\n");
    if (!BL_Raw_Copy(SCRATCH_ADDR, APP_ACTIVE_START_ADDR, SLOT_SIZE)) {
        printf("Error: Installation Failed.\r\n");
        HAL_FLASH_Lock();
        return 4;
    }

    printf("[BL] Rollback Successful! Resetting...\r\n");
    cfg.system_status = STATE_NORMAL;
    BL_WriteConfig(&cfg);
    HAL_NVIC_SystemReset();
}
