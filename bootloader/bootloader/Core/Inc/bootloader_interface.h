/*
 * bootloader_interface.h
 *
 *  Created on: Dec 2, 2025
 *      Author: mertk
 */

#ifndef INC_BOOTLOADER_INTERFACE_H_
#define INC_BOOTLOADER_INTERFACE_H_

#include <stdint.h>

typedef struct {
    uint32_t magic_code;      // 0xDEADBEEF
    uint32_t version;         // Bootloader Version

    /* Function Pointers */
    void (*RequestUpdate)(void);
    uint32_t (*GetBootStatus)(void);

    /* Returns 1 if valid, 0 if invalid */
    int (*VerifySlot)(uint32_t slot_id);
} Bootloader_API_t;

#endif /* INC_BOOTLOADER_INTERFACE_H_ */
