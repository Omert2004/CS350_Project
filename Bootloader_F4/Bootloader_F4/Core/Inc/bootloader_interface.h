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



// --- 2. System States (Logical States) ---
typedef enum {
    STATE_NORMAL       = 4,      // Normal Boot
    STATE_UPDATE_REQ   = 5,      // App requests update
    STATE_ROLLBACK     = 6       // Update failed, rolling back
} BL_System_Status_t;

typedef struct {
    uint32_t magic_number;   // 0xDEADBEEF
    BL_System_Status_t system_status;  // STATE_NORMAL, UPDATE_REQ, or ROLLBACK
    uint32_t current_version;
} BootConfig_t;


// --- 4. The "Connection" Macro ---
// This allows code to access the config variables directly from Flash
#define BOOT_CONFIG  ((volatile BootConfig_t *)CONFIG_SECTOR_ADDR)


#endif /* INC_BOOTLOADER_INTERFACE_H_ */
