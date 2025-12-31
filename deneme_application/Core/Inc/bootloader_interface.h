/*
 * bootloader_interface.h
 *
 *  Created on: 31 Ara 2025
 *      Author: Oguzm
 */

/*
 * bootloader_interface.h
 * Shared definition file for Bootloader and Application
 */

#ifndef INC_BOOTLOADER_INTERFACE_H_
#define INC_BOOTLOADER_INTERFACE_H_

#include <stdint.h>

// --- 1. Memory Layout (Physical Addresses) ---
// Bootloader: 0x08000000 (64KB - Sectors 0-1)
// Config:     0x08010000 (32KB - Sector 2)
#define CONFIG_SECTOR_ADDR       0x08010000
#define CONFIG_SECTOR_NUM        FLASH_SECTOR_2

// Application Slots
#define APP_ACTIVE_START_ADDR    0x08040000 // Sector 5
#define APP_DOWNLOAD_START_ADDR  0x08080000 // Sector 6
#define SCRATCH_ADDR             0x080C0000 // Sector 7 (Scratch)

#define SLOT_SIZE                0x00040000 // 256 KB

// --- 2. System States (Logical States) ---
typedef enum {
    STATE_NORMAL       = 0,      // Normal Boot
    STATE_UPDATE_REQ   = 1,      // App requests update
    STATE_TESTING      = 2,      // Testing new update
    STATE_ROLLBACK     = 3       // Update failed, rolling back
} BL_System_Status_t;

// --- 3. Configuration Structure ---
// This struct sits exactly at 0x08010000
typedef struct {
    uint32_t magic_number;       // 0xDEADBEEF (Validity Check)
    uint32_t system_status;      // BL_System_Status_t
    uint32_t boot_failure_count; // Reliability counter
    uint32_t active_slot;        // 1 or 2 (which slot is active?)
    uint32_t current_version;    // Version of current app
    uint8_t  padding[12];        // Padding to align to 32 bytes
} BootConfig_t;

// --- 4. The "Connection" Macro ---
// This allows code to access the config variables directly from Flash
#define BOOT_CONFIG  ((volatile BootConfig_t *)CONFIG_SECTOR_ADDR)


#endif /* INC_BOOTLOADER_INTERFACE_H_ */
