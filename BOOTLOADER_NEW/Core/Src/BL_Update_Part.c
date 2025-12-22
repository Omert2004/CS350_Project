/*
 * BL_Update_Part.c
 *
 *  Created on: 22 Ara 2025
 *      Author: Oguzm
 */

#include "main.h"
#include "BL_Update_Part.h"
#include <string.h>          // [Fix: Needed for memcpy]
#include "mem_layout.h"      // [Fix: Needed for APP_..._ADDR defines]
#include "BL_Functions.h"    // [Fix: Needed for Bootloader_InternalVerify]
#include "firmware_footer.h" // [Fix: Needed for BL_Status_t]

// Libs Includes
#include "aes.h"
#include "cbc_mode.h"
#include "lz4.h"

// [Fix: Define Constants that were missing]
#define ENC_CHUNK_SIZE 1024
#define DEC_CHUNK_SIZE 1024
#define RAW_CHUNK_SIZE 4096  // LZ4 output might be larger than input

// [Fix: Define Backup Register Status Flags]
#define BL_STATUS_ERROR    0xDEADDEAD
#define BL_STATUS_UPDATED  0xAAAAAAAA

// --- EXTERNAL VARIABLES (Fixes 'undeclared' errors) ---
// These tell the compiler: "Trust me, these exist in main.c and keys.c"
extern RTC_HandleTypeDef hrtc;           // Defined in main.c
extern const uint8_t AES_SECRET_KEY[16]; // Defined in keys.c

void Bootloader_SetStatus(uint32_t status) {
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, status);
    HAL_PWREx_DisableBkUpReg();
}

void BL_RequestUpdate(void) {
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0xCAFEBABE); // Magic Flag
    NVIC_SystemReset();
    HAL_PWREx_DisableBkUpReg();
}

uint32_t BL_GetStatus(void) {
    return HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
}

/* Returns 1 on Success, 0 on Failure */
int Install_Update_Stream(uint8_t is_dry_run) {
    static uint8_t enc_buffer[ENC_CHUNK_SIZE];
    static uint8_t dec_buffer[DEC_CHUNK_SIZE];
    static uint8_t raw_buffer[RAW_CHUNK_SIZE];

    uint32_t read_addr = APP_DOWNLOAD_START_ADDR;
    uint32_t write_addr = APP_ACTIVE_START_ADDR;

    /* 1. Setup AES */
    struct tc_aes_key_sched_struct sched;
    uint8_t iv[16];
    uint8_t next_iv[16];

    /* Read Initial IV */
    memcpy(iv, (void*)read_addr, 16);
    read_addr += 16;
    tc_aes128_set_decrypt_key(&sched, AES_SECRET_KEY);

    /* 2. Process Loop */
    // Limit loop to Download Size to prevent reading garbage at end of flash
    uint32_t end_addr = APP_DOWNLOAD_START_ADDR + APP_DOWNLOAD_SIZE;

    while (read_addr < end_addr) {

        // A. Read Encrypted Chunk
        memcpy(enc_buffer, (void*)read_addr, ENC_CHUNK_SIZE);

        // Check for empty flash (0xFF) to stop early
        if (*(uint32_t*)enc_buffer == 0xFFFFFFFF) break;

        // B. Save IV for next chunk (CBC Chain)
        memcpy(next_iv, &enc_buffer[ENC_CHUNK_SIZE - 16], 16);

        // C. Decrypt
		if (tc_cbc_mode_decrypt(dec_buffer, ENC_CHUNK_SIZE, enc_buffer, ENC_CHUNK_SIZE, iv, &sched) == 0) {
			 return 0; // Decrypt Error
		}
		memcpy(iv, next_iv, 16); // Update IV

		// --- NEW PROTOCOL HANDLING ---
		// 1. Read the compressed size from the first 2 bytes
		uint16_t compressed_len = *(uint16_t*)dec_buffer;

		// 2. Sanity check (Size must be reasonable)
		if (compressed_len > (ENC_CHUNK_SIZE - 2) || compressed_len == 0) {
			return 0; // Corruption or attack
		}

		// D. Decompress (LZ4)
		// Note: Input is &dec_buffer[2] (skipping the size header)
		//       Input Size is compressed_len (exact size)
		int bytes_out = LZ4_decompress_safe((const char*)&dec_buffer[2],
											(char*)raw_buffer,
											compressed_len,
											RAW_CHUNK_SIZE);

		if (bytes_out < 0) return 0; // CORRUPTION DETECTED!

        // E. Write (ONLY IF NOT DRY RUN)
        if (!is_dry_run) {
            for (int i = 0; i < bytes_out; i += 4) {
                if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, write_addr + i, *(uint32_t*)(raw_buffer + i)) != HAL_OK) {
                    return 0; // Flash Write Error
                }
            }
        }

        read_addr += ENC_CHUNK_SIZE;
        write_addr += bytes_out;
    }

    return 1; // Stream is valid
}

void Bootloader_HandleUpdate(void) {

    // --- STEP 1: SAFETY CHECK (DRY RUN) ---
    // We process the whole file but DO NOT erase or write anything.
    // This checks if the file is corrupt, truncated, or has the wrong key.

    if (Install_Update_Stream(1) == 0) {
        // DRY RUN FAILED!
        // The download slot contains garbage.
        // We abort immediately. The Active Slot is still perfectly valid.
        Bootloader_SetStatus(BL_STATUS_ERROR);

        // Optional: Erase the bad download slot so we don't try again
        // Erase_Download_Slot();

        NVIC_SystemReset(); // Reboot back to old App
        return;
    }

    // --- STEP 2: PREPARE ACTIVE SLOT ---
    // If we got here, we know the Download Slot is readable and valid LZ4.

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef EraseInit;
    uint32_t SectorError;
    EraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInit.Sector = FLASH_SECTOR_2;
    EraseInit.NbSectors = 4; // Sectors 2,3,4,5
    EraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&EraseInit, &SectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        Error_Handler(); // Hardware failure
    }

    // --- STEP 3: REAL INSTALLATION ---
    // This time we pass '0' to enable writing.
    if (Install_Update_Stream(0) == 1) {
        HAL_FLASH_Lock();

        // --- STEP 4: FINAL SIGNATURE VERIFY ---
        // We verified the stream structure (LZ4), but now we verify
        // the cryptographic signature of the code we just wrote.
        if (Bootloader_InternalVerify(APP_ACTIVE_START_ADDR, APP_ACTIVE_SIZE) == 1) {
            Bootloader_SetStatus(BL_STATUS_UPDATED);
            NVIC_SystemReset();
        } else {
            // Signature mismatch (Malicious file?)
            // We are now bricked (Active Slot is erased/written but invalid).
            Error_Handler();
        }
    } else {
        // Write failed halfway?
        HAL_FLASH_Lock();
        Error_Handler();
    }
}

