/*
 * mem_layout.h
 *
 * @brief   Memory Map definitions for STM32F746 Secure Bootloader.
 * @details Updated Layout: Bootloader uses Sectors 0 & 1 (64KB).
 */

#ifndef INC_MEM_LAYOUT_H_
#define INC_MEM_LAYOUT_H_

#include <stdint.h>

/*
 * STM32F411 (512KB) Flash Layout Map
 *
 * Sector layout:
 * S0  0x08000000 - 0x08003FFF  16 KB (Bootloader)
 * S1  0x08004000 - 0x08007FFF  16 KB (Bootloader)
 * S2  0x08008000 - 0x0800BFFF  16 KB (App Start - Now Available)
 * S3  0x0800C000 - 0x0800FFFF  16 KB
 * ...
 */

/* 1. Bootloader (32 KB) - Sectors 0 & 1 */
#define BOOTLOADER_START_ADDR    0x08000000
#define BOOTLOADER_SIZE          0x00008000  /* 32 KB = 0x8000 */

/* Shared API Address (Fixed at Sector 0 offset 0x200) */
#define SHARED_API_ADDRESS       0x08000200

/* 2. Config Sector (32 KB) - Uses Sector 2 */
/* Moved here so we don't erase bootloader code */
#define CONFIG_SECTOR_ADDR       0x08008000
#define CONFIG_SECTOR_NUM        FLASH_SECTOR_2

/* 2. Active Application Slot - Starts at Sector 2 upto 5 (included)*/
/* */
#define APP_ACTIVE_START_ADDR    0x08020000
#define SLOT_A_SECTOR            0x00020000  /* 128KB = 1 0000*/
/* 192 KB (0x30000), 64 = 1 0000, 128 = 2 0000 */
// 1 0000 + 2 0000 = 3 0000 => 192 KB

/* 3. Download / Inactive Slot - Sectors 5, 6 (256 KB) */
#define APP_DOWNLOAD_START_ADDR  0x08040000  //
#define APP_DOWNLOAD_SIZE        0x00020000//   0x00040000 = (256KB)

#define SCRATCH_ADDR         	0x08060000  // Sector 7 (Buffer)
#define SCRATCH_SECTOR       	FLASH_SECTOR_7

#define SLOT_SIZE            0x00020000  // 128 KB


#endif /* INC_MEM_LAYOUT_H_ */
