/*
 * mem_layout.h
 *
 * @brief   Memory Map definitions for STM32F746 Secure Bootloader.
 *
 * @details This file defines the flash memory partition between the Bootloader
 * and the Application. The addresses align with the STM32F7 physical
 * sector boundaries to ensure safe erase operations.
 *
 * @note    These definitions must match the Linker Script (.ld) configuration.c
 *
 *  Created on: Dec 2, 2025
 *      Author: mertk
 */

#ifndef INC_MEM_LAYOUT_H_
#define INC_MEM_LAYOUT_H_

/*
 * STM32F746G (1MB) Flash Layout Map
 *
 * Sector layout:
 * S0  0x08000000 - 0x08007FFF  32 KB
 * S1  0x08008000 - 0x0800FFFF  32 KB
 * S2  0x08010000 - 0x08017FFF  32 KB
 * S3  0x08018000 - 0x0801FFFF  32 KB
 * S4  0x08020000 - 0x0803FFFF 128 KB
 * S5  0x08040000 - 0x0807FFFF 256 KB
 * S6  0x08080000 - 0x080BFFFF 256 KB
 * S7  0x080C0000 - 0x080FFFFF 256 KB
 */

/* 1. Bootloader (96 KB) - Sectors 0, 1, 2 */
#define BOOTLOADER_START_ADDR    0x08000000
#define BOOTLOADER_SIZE          0x00018000  /* 96 KB */

/* Optional: shared API placed at the END of bootloader */
#define SHARED_API_ADDRESS       0x08017000  /* inside Sector 2 */

/* 2. Active Application Slot - Sectors 3, 4, 5 (416 KB) */
#define APP_ACTIVE_START_ADDR    0x08018000
#define APP_ACTIVE_SIZE          0x00068000  /* S3 + S4 + part of S5 */

/* 3. Download / Inactive Slot - Sectors 6, 7 (512 KB) */
#define APP_DOWNLOAD_START_ADDR  0x08080000
#define APP_DOWNLOAD_SIZE        0x00080000

#endif /* INC_MEM_LAYOUT_H_ */
