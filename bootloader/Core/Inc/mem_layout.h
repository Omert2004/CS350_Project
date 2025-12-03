/*
 * mem_layout.h
 *
 *  Created on: Dec 2, 2025
 *      Author: mertk
 */

#ifndef INC_MEM_LAYOUT_H_
#define INC_MEM_LAYOUT_H_
// BURASI DÜZELTİLECEK  YANLIŞ SAYILAR VAR!!!!!!!!!!!!!!!!!!!!!!!!
/* STM32F746G (1MB) Flash Layout Map */

/* 1. Bootloader (64KB) - Sectors 0 & 1 */
#define BOOTLOADER_START_ADDR   0x08000000
#define BOOTLOADER_SIZE         0x00010000

/* 2. Shared API (Placed at end of Bootloader region) */
/* This specific address MUST match the .ld linker script! */
#define SHARED_API_ADDRESS      0x0800F000

/* 3. Active Slot (App) - Sectors 2, 3, 4, 5 (~448KB) */
#define APP_ACTIVE_START_ADDR   0x08010000
#define APP_ACTIVE_SIZE         0x00070000

/* 4. Download/Inactive Slot - Sectors 6, 7 (512KB) */
#define APP_DOWNLOAD_START_ADDR 0x08080000
#define APP_DOWNLOAD_SIZE       0x00080000

#endif /* INC_MEM_LAYOUT_H_ */
