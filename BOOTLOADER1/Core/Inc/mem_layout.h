/*
 * mem_layout.h
 *
 * @brief   Memory Map definitions for STM32F746 Secure Bootloader.
 * @details Updated Layout: Bootloader uses Sectors 0 & 1 (64KB).
 */
#ifndef INC_MEM_LAYOUT_H_
#define INC_MEM_LAYOUT_H_

#include <stdint.h>

/* 1. Bootloader (64 KB) - Uses Sectors 0 & 1 */
#define BOOTLOADER_START_ADDR    0x08000000
#define BOOTLOADER_SIZE          0x00010000  // 64 KB (Covers S0 and S1)

/* Shared API (Inside Bootloader S0) */
#define SHARED_API_ADDRESS       0x08000200

/* 2. Config Sector (32 KB) - Uses Sector 2 */
/* Moved here so we don't erase bootloader code */
#define CONFIG_SECTOR_ADDR       0x08010000
#define CONFIG_SECTOR_NUM        FLASH_SECTOR_2


#define APP_ACTIVE_START_ADDR   0x08040000  // Sector 5 (Active)
#define SLOT_A_SECTOR        	FLASH_SECTOR_5

#define APP_DOWNLOAD_START_ADDR 0x08080000  // Sector 6 (Download)
#define SLOT_B_SECTOR        	FLASH_SECTOR_6

#define SCRATCH_ADDR         	0x080C0000  // Sector 7 (Buffer)
#define SCRATCH_SECTOR       	FLASH_SECTOR_7

#define SLOT_SIZE            0x00040000  // 256 KB


#endif /* INC_MEM_LAYOUT_H_ */
