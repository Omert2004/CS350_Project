/*
 * mem_layout.h
 *
 * @brief   Memory Map definitions for STM32F746 Secure Bootloader.
 * @details Updated Layout: Bootloader uses Sectors 0 & 1 (64KB).
 */

#ifndef INC_MEM_LAYOUT_H_
#define INC_MEM_LAYOUT_H_

/*
 * STM32F746G (1MB) Flash Layout Map
 *
 * Sector layout:
 * S0  0x08000000 - 0x08007FFF  32 KB (Bootloader)
 * S1  0x08008000 - 0x0800FFFF  32 KB (Bootloader)
 * S2  0x08010000 - 0x08017FFF  32 KB (App Start - Now Available)
 * S3  0x08018000 - 0x0801FFFF  32 KB
 * ...
 */

/* 1. Bootloader (64 KB) - Sectors 0 & 1 */
#define BOOTLOADER_START_ADDR    0x08000000
#define BOOTLOADER_SIZE          0x00010000  /* 64 KB (Modified from 96KB) */

/* Shared API Address (Fixed at Sector 0 offset 0x200) */
#define SHARED_API_ADDRESS       0x08000200

/* 2. Active Application Slot - Starts at Sector 2 */
/* * NOT: Bootloader'ı 64KB'a çektiğimiz için Application artık Sector 2'den başlayabilir.
 * Eski adres: 0x08018000 (Sector 3) idi.
 * Yeni adres: 0x08010000 (Sector 2)
 */
#define APP_ACTIVE_START_ADDR    0x08010000
#define APP_ACTIVE_SIZE          0x00070000  /* 448 KB (Increased by 32KB) */

/* 3. Download / Inactive Slot - Sectors 6, 7 (512 KB) */
#define APP_DOWNLOAD_START_ADDR  0x08080000
#define APP_DOWNLOAD_SIZE        0x00080000

#endif /* INC_MEM_LAYOUT_H_ */
