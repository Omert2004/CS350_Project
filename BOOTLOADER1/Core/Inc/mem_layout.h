/*
 * mem_layout.h
 *
 * @brief   Memory Map definitions for STM32F746 Secure Bootloader.
 * @details Updated Layout: Bootloader uses Sectors 0 & 1 (64KB).
 */
#ifndef INC_MEM_LAYOUT_H_
#define INC_MEM_LAYOUT_H_

/*
 * STM32F746G (1MB) Flash Layout Map (CORRECTED)
 *
 * S0  (32KB)  : 0x08000000 - 0x08007FFF  -> Bootloader Code Part 1
 * S1  (32KB)  : 0x08008000 - 0x0800FFFF  -> Bootloader Code Part 2
 * S2  (32KB)  : 0x08010000 - 0x08017FFF  -> CONFIGURATION (State Flags)
 * S3  (32KB)  : 0x08018000 - 0x0801FFFF  -> App Start (ISR Vector)
 * S4  (128KB) : 0x08020000 - 0x0803FFFF  -> App Code
 * S5  (256KB) : 0x08040000 - 0x0807FFFF  -> App Code
 * S6  (256KB) : 0x08080000 ...           -> Download Slot
 * S7  (256KB) : ...                      -> Download Slot
 */

/* 1. Bootloader (64 KB) - Uses Sectors 0 & 1 */
#define BOOTLOADER_START_ADDR    0x08000000
#define BOOTLOADER_SIZE          0x00010000  // 64 KB (Covers S0 and S1)

/* Shared API (Inside Bootloader S0) */
#define SHARED_API_ADDRESS       0x08000200

/* 2. Config Sector (32 KB) - Uses Sector 2 */
/* Moved here so we don't erase bootloader code */
#define CONFIG_SECTOR_ADDR       0x08010000
#define CONFIG_SECTOR_NUM        FLASH_SECTOR_2

/* 3. Active App (416 KB) - Uses Sectors 3, 4, 5 */
/* START MOVED TO SECTOR 3 */
#define APP_ACTIVE_START_ADDR    0x08018000
#define APP_ACTIVE_SIZE          0x00068000  // 32K(S3) + 128K(S4) + 256K(S5) = 416KB

/* 4. Download / Backup Slot (512 KB) - Sectors 6, 7 */
#define APP_DOWNLOAD_START_ADDR  0x08080000
#define APP_DOWNLOAD_SIZE        0x00080000  // 512 KB

#endif /* INC_MEM_LAYOUT_H_ */
