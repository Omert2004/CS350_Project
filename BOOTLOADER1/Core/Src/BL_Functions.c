/*
 * BL_Functions.c
 *
 * Modified to support "Pad First, Sign Later" and CBC Decryption
 */

#include "BL_Functions.h"
#include "aes.h"
#include "keys.h"
#include "mem_layout.h"
#include "tiny_printf.h"
#include "stm32f7xx_hal.h"
#include "string.h"
#include "Cryptology_Control.h"

extern const uint8_t AES_SECRET_KEY[];

// --- 1. Define the Header Structure (Must match Python) ---
typedef struct {
    uint32_t magic;        // 0xDEADBEEF
    uint32_t version;      // 2
    uint32_t fw_size;      // Size used for Verification (Padded)
    uint32_t padded_size;  // Total Encrypted Size
    uint8_t  iv[16];       // Initialization Vector for CBC
    uint8_t  reserved[32];
} Update_Header_t; // Total Size: 64 Bytes

#define HEADER_SIZE 64
#define SIG_SIZE    64
#define META_DATA_SIZE (HEADER_SIZE + SIG_SIZE) // 128 Bytes to skip before data

// --- Config Functions (Keep these as they were) ---

uint8_t BL_ReadConfig(BootConfig_t *cfg) {
    uint8_t config_was_empty = 0;
    memcpy(cfg, (void *)CONFIG_SECTOR_ADDR, sizeof(BootConfig_t));

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

uint8_t BL_WriteConfig(BootConfig_t *cfg) {
    uint32_t SectorError;
    uint32_t *data_ptr = (uint32_t *)cfg;
    int num_words = sizeof(BootConfig_t) / 4;

    __disable_irq();
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase_init = {0};
    erase_init.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase_init.Sector       = CONFIG_SECTOR_NUM;
    erase_init.NbSectors    = 1;

    if (HAL_FLASHEx_Erase(&erase_init, &SectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        __enable_irq();
        return 0;
    }

    for (int i = 0; i < num_words; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                              CONFIG_SECTOR_ADDR + (i * 4),
                              data_ptr[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            __enable_irq();
            return 0;
        }
    }

    HAL_FLASH_Lock();
    __enable_irq();
    return 1;
}

// --- Copy Functions ---

static uint8_t BL_Raw_Copy(uint32_t src_addr, uint32_t dest_addr, uint32_t size){
    __disable_irq();
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    if (dest_addr == APP_ACTIVE_START_ADDR) erase.Sector = FLASH_SECTOR_5;
    else if (dest_addr == APP_DOWNLOAD_START_ADDR) erase.Sector = FLASH_SECTOR_6;
    else erase.Sector = FLASH_SECTOR_7;

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
        __enable_irq(); return 0;
    }

    // Write Loop
    for (uint32_t i = 0; (i * 4) < size; i++) {
    	uint32_t data = *(uint32_t*)(src_addr + (i * 4));
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest_addr + (i * 4), data) != HAL_OK) {
            __enable_irq(); return 0;
        }
    }

    __enable_irq();
    return 1;
}

/**
 * @brief Decrypts an UPDATE package (CBC Mode) from Source to Dest
 * Reads Header/IV from Source, Skips Header+Sig, Decrypts Data -> Dest
 */
static uint8_t BL_Decrypt_Update_Image(uint32_t src_slot_addr, uint32_t dest_addr, uint32_t data_size, uint8_t* iv) {
    struct tc_aes_key_sched_struct s;
    uint8_t buffer_enc[16];
    uint8_t buffer_dec[16];
    uint8_t current_iv[16]; // CBC IV State
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    // Load Key
    if (tc_aes128_set_decrypt_key(&s, AES_SECRET_KEY) == 0) return 0;

    // Initialize CBC IV
    memcpy(current_iv, iv, 16);

    __disable_irq();
    HAL_FLASH_Unlock();

    // Erase Destination (Usually S7)
    if (dest_addr == APP_ACTIVE_START_ADDR) erase.Sector = FLASH_SECTOR_5;
    else if (dest_addr == APP_DOWNLOAD_START_ADDR) erase.Sector = FLASH_SECTOR_6;
    else erase.Sector = FLASH_SECTOR_7;

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
        HAL_FLASH_Lock(); __enable_irq(); return 0;
    }

    // Offset starts AFTER Header + Signature
    uint32_t data_start_offset = META_DATA_SIZE;

    // Decrypt Loop (CBC Mode)
    // 1. Read Ciphertext Block
    // 2. AES Decrypt -> Intermediate
    // 3. Plaintext = Intermediate XOR IV
    // 4. IV = Ciphertext Block (for next round)

    for (uint32_t i = 0; i < data_size; i += 16) {
        // Read Encrypted Block from Flash
        memcpy(buffer_enc, (void*)(src_slot_addr + data_start_offset + i), 16);

        // AES ECB Decrypt primitive
        if (tc_aes_decrypt(buffer_dec, buffer_enc, &s) == 0) {
             HAL_FLASH_Lock(); __enable_irq(); return 0;
        }

        // Apply CBC XOR
        for(int j=0; j<16; j++) {
            buffer_dec[j] ^= current_iv[j];
        }

        // Update IV for next block (Next IV is the current Ciphertext)
        memcpy(current_iv, buffer_enc, 16);

        // Write Decrypted data to Flash
        for (int k = 0; k < 4; k++) {
            uint32_t data_word;
            memcpy(&data_word, &buffer_dec[k*4], 4);
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest_addr + i + (k*4), data_word) != HAL_OK) {
                HAL_FLASH_Lock(); __enable_irq(); return 0;
            }
        }
    }
    HAL_FLASH_Lock(); __enable_irq(); return 1;
}

// Backup (S5 -> S6) - Keeps using ECB (No Header) as per original design for internal backup
static uint8_t BL_Encrypt_Backup(uint32_t src_addr, uint32_t dest_addr) {
    struct tc_aes_key_sched_struct s;
    uint8_t buffer_plain[16];
    uint8_t buffer_enc[16];
    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    if (tc_aes128_set_encrypt_key(&s, AES_SECRET_KEY) == 0) return 0;

    __disable_irq();
    HAL_FLASH_Unlock();

    erase.Sector = FLASH_SECTOR_6;
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.NbSectors = 1;

    if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
        HAL_FLASH_Lock(); __enable_irq(); return 0;
    }

    // Encrypts 128KB (SLOT_SIZE)
    for (uint32_t offset = 0; offset < SLOT_SIZE; offset += 16) {
        memcpy(buffer_plain, (void*)(src_addr + offset), 16);
        if (tc_aes_encrypt(buffer_enc, buffer_plain, &s) == 0) {
             HAL_FLASH_Lock(); __enable_irq(); return 0;
        }
        for (int i = 0; i < 4; i++) {
            uint32_t data_word;
            memcpy(&data_word, &buffer_enc[i*4], 4);
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest_addr + offset + (i*4), data_word) != HAL_OK) {
                HAL_FLASH_Lock(); __enable_irq(); return 0;
            }
        }
    }
    HAL_FLASH_Lock(); __enable_irq(); return 1;
}

// --- Main Swap Function ---
void BL_Swap_NoBuffer(void) {

	BootConfig_t cfg;
	BL_ReadConfig(&cfg);

	// 1. READ HEADER FROM S6
	Update_Header_t header;
	memcpy(&header, (void*)APP_DOWNLOAD_START_ADDR, sizeof(Update_Header_t));

	if (header.magic != 0xDEADBEEF) {
		printf("Error: Invalid Update Header in S6.\r\n");
		// Optional: Could revert to state normal here
		return;
	}

	// Cast to int for tiny_printf support
	printf("[BL] Update found! Ver: %d, Size: %d\r\n", (int)header.version, (int)header.fw_size);

	HAL_FLASH_Unlock();
    printf("Starting Direct Swap (Zero-RAM Mode)...\r\n");

    // 2. Decrypt S6 -> S7 (Using CBC + Header Info)
    printf("[1/3] Decrypting S6 -> S7...\r\n");
    // We pass the IV from the header and the Padded Size
	if (!BL_Decrypt_Update_Image(APP_DOWNLOAD_START_ADDR, SCRATCH_ADDR, header.padded_size, header.iv)) {
		printf("Error: Decryption Failed.\r\n");
		return;
	}

	// 3. Verify S7
	// CRITICAL: We verify using the size from the header!
	// Also pass the signature which is stored in S6 at offset 64
	// (Assuming Firmware_Is_Valid can take a signature argument, OR if it reads from S7 footer)
	// If Firmware_Is_Valid reads the internal footer, this works because the footer is inside the encrypted blob.
	// If you use external signature (from python), you might need to adjust Firmware_Is_Valid.

	// Assuming your Firmware_Is_Valid checks the hash of the content at S7:
	// We check 'header.fw_size' (which includes padding in your new python script)
	if (Firmware_Is_Valid(SCRATCH_ADDR, header.fw_size) != 1) {
		printf("CRITICAL ERROR: Signature Verification Failed!\r\n");

		// Wipe S7
		FLASH_EraseInitTypeDef erase;
		uint32_t error;
		erase.TypeErase = FLASH_TYPEERASE_SECTORS;
		erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
		erase.Sector = FLASH_SECTOR_7;
		erase.NbSectors = 1;

		HAL_FLASH_Unlock();
		HAL_FLASHEx_Erase(&erase, &error);
		HAL_FLASH_Lock();

		cfg.system_status = STATE_NORMAL;
		BL_WriteConfig(&cfg);
		return;
	}
	printf("Verification Successful. Firmware is trusted.\r\n");

    // 4. Backup S5 -> S6 (Standard Encrypt)
	printf("[2/3] Encrypting & Backing up S5 -> S6...\r\n");
	if (!BL_Encrypt_Backup(APP_ACTIVE_START_ADDR, APP_DOWNLOAD_START_ADDR)) {
		printf("Error: Backup Encryption Failed.\r\n");
		return;
	}

	// 5. Install S7 -> S5
	printf("[3/3] Installing S7 -> S5...\r\n");
	// Use fw_size to copy exactly what we need (or SLOT_SIZE to be safe/lazy)
	if (!BL_Raw_Copy(SCRATCH_ADDR, APP_ACTIVE_START_ADDR, SLOT_SIZE)) {
		printf("Error: Installation Failed.\r\n");
		return;
	}

    HAL_FLASH_Lock();
    printf("Swap Complete.\r\n");
}
