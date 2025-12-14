/*
 * bootloader_interface.h
 *
 *@brief   Public API definition for the Secure Bootloader.
 *
 * @details This header is shared between the Bootloader and the User Application.
 * It defines the structure used to expose internal Bootloader functions
 * (like update requests and verification) to the running Application
 * via a fixed memory address.
 *
 *  Created on: Dec 2, 2025
 *      Author: mertk
 */

#ifndef INC_BOOTLOADER_INTERFACE_H_
#define INC_BOOTLOADER_INTERFACE_H_

#include <stdint.h>

typedef struct {
	/**
	     * @brief Magic Code (0xDEADBEEF)
	     * @details Used by the Application to verify that the Bootloader is present
	     * and the API table is valid before calling any functions.
	*/
    uint32_t magic_code;      // 0xDEADBEEF
    uint32_t version;         // Bootloader Version

    /* Function Pointers */
    void (*RequestUpdate)(void);
    uint32_t (*GetBootStatus)(void);

    /* Returns 1 if valid, 0 if invalid */
    int (*VerifySlot)(uint32_t slot_id);
} Bootloader_API_t;

#endif /* INC_BOOTLOADER_INTERFACE_H_ */
