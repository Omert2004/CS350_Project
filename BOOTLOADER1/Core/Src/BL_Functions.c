/*
 * BL_Functions.c - Debug Version
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

// --- Config Functions ---
uint8_t BL_ReadConfig(BootConfig_t *cfg) {
    memcpy(cfg, (void *)CONFIG_SECTOR_ADDR, sizeof(BootConfig_t));
    if (cfg->magic_number != 0xDEADBEEF) {
        cfg->magic_number = 0xDEADBEEF;
        cfg->system_status = STATE_NORMAL;
        cfg->active_slot = 1;
        return 1;
    }
    return 0;
}

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
    for (int i = 0; i < sizeof(BootConfig_t)/4; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, CONFIG_SECTOR_ADDR + (i*4), ((uint32_t*)cfg)[i]);
    }
    HAL_FLASH_Lock(); return 1;
}

static uint8_t BL_Raw_Copy(uint32_t src_addr, uint32_t dest_addr, uint32_t size){
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    // 1. Select Sector
    if (dest_addr == APP_ACTIVE_START_ADDR) erase.Sector = FLASH_SECTOR_5;
    else if (dest_addr == APP_DOWNLOAD_START_ADDR) erase.Sector = FLASH_SECTOR_6;
    else erase.Sector = FLASH_SECTOR_7;

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    // 2. Erase
    // CHANGED: %lu -> %d and cast to (int)
    printf("  [DEBUG] Erasing Sector %d... ", (int)erase.Sector);

    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
        // CHANGED: %lx -> %x and cast to (unsigned int)
        // Also printing FLASH->SR register directly for more detail
        printf("FAILED! HAL Error: 0x%x (SR: 0x%x)\r\n",
               (unsigned int)HAL_FLASH_GetError(),
               (unsigned int)FLASH->SR);
        HAL_FLASH_Lock(); return 0;
    }
    printf("OK\r\n");

    // 3. Write Loop
    printf("  [DEBUG] Writing %d bytes to 0x%x... ", (int)size, (unsigned int)dest_addr);

    for (uint32_t i = 0; (i * 4) < size; i++) {
        uint32_t data = *(uint32_t*)(src_addr + (i * 4));

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest_addr + (i * 4), data) != HAL_OK) {
            // CHANGED: Formats updated
            printf("\r\nFAILED at offset %d! HAL Err: 0x%x (SR: 0x%x)\r\n",
                   (int)(i*4),
                   (unsigned int)HAL_FLASH_GetError(),
                   (unsigned int)FLASH->SR);
            HAL_FLASH_Lock(); return 0;
        }
    }
    printf("OK\r\n");

    HAL_FLASH_Lock();
    return 1;
}

// --- Decrypt Function ---
static uint8_t BL_Decrypt_Update_Image(uint32_t src_slot_addr, uint32_t dest_addr, uint32_t payload_size) {
    struct tc_aes_key_sched_struct s;
    uint8_t buffer_enc[16], buffer_dec[16], current_iv[16];
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    if (tc_aes128_set_decrypt_key(&s, AES_SECRET_KEY) == 0) {
        printf("Error: AES Key Invalid.\r\n"); return 0;
    }

    memcpy(current_iv, (void*)src_slot_addr, 16); // Read IV

    HAL_FLASH_Unlock();
    erase.Sector = FLASH_SECTOR_7; // S7 is usually scratch
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
        printf("Error: Failed to erase Scratch (S7).\r\n");
        HAL_FLASH_Lock(); return 0;
    }

    uint32_t encrypted_data_size = payload_size - 16;
    uint32_t data_start_offset = 16;

    for (uint32_t i = 0; i < encrypted_data_size; i += 16) {
        memcpy(buffer_enc, (void*)(src_slot_addr + data_start_offset + i), 16);
        tc_aes_decrypt(buffer_dec, buffer_enc, &s);

        for(int j=0; j<16; j++) buffer_dec[j] ^= current_iv[j]; // CBC XOR
        memcpy(current_iv, buffer_enc, 16); // Update IV

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

static uint8_t BL_Encrypt_Backup(uint32_t src_addr, uint32_t dest_addr) {
    // Simplified for brevity, assume standard backup works or use previous version
    // Ideally this also checks for HAL Errors like BL_Raw_Copy
    return BL_Raw_Copy(src_addr, dest_addr, SLOT_SIZE); // Using Raw Copy for now to test S5 issue
}

void BL_Swap_NoBuffer(void) {
    BootConfig_t cfg;
    BL_ReadConfig(&cfg);

    uint32_t footer_addr = Find_Footer_Address(APP_DOWNLOAD_START_ADDR, SLOT_SIZE);
    if (footer_addr == 0) { printf("Error: No Footer found.\r\n"); return; }

    fw_footer_t footer;
    memcpy(&footer, (void*)footer_addr, sizeof(fw_footer_t));

    printf("[BL] Verifying Signature... ");
    if (Firmware_Is_Valid(APP_DOWNLOAD_START_ADDR, SLOT_SIZE) != 1) {
        printf("FAIL!\r\n"); return;
    }
    printf("OK!\r\n");
    printf("[BL] Valid Update! Ver: %d, Payload: %d\r\n", (int)footer.version, (int)footer.size);

    printf("[1/3] Decrypting S6 -> S7...\r\n");
    if (!BL_Decrypt_Update_Image(APP_DOWNLOAD_START_ADDR, SCRATCH_ADDR, footer.size)) {
        printf("Error: Decryption Failed.\r\n"); return;
    }

    printf("[2/3] Backing up S5 -> S6...\r\n");
    // Just raw copy S5 to S6 for backup test
    if (!BL_Raw_Copy(APP_ACTIVE_START_ADDR, APP_DOWNLOAD_START_ADDR, SLOT_SIZE)) {
        printf("Error: Backup Failed.\r\n"); return;
    }

    printf("[3/3] Installing S7 -> S5...\r\n");
    if (!BL_Raw_Copy(SCRATCH_ADDR, APP_ACTIVE_START_ADDR, SLOT_SIZE)) {
        printf("Error: Installation Failed.\r\n");
        // Don't revert config yet, let user see error
        return;
    }

    printf("Swap Complete. Resetting...\r\n");
    HAL_NVIC_SystemReset();
}
