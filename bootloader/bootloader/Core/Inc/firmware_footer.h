/*
 * firmware_footer.h
 *
 *  Created on: Dec 2, 2025
 *      Author: mertk
 */

#ifndef INC_FIRMWARE_FOOTER_H_
#define INC_FIRMWARE_FOOTER_H_

#ifndef FW_FOOTER_H
#define FW_FOOTER_H

#include <stdint.h>

/* This Magic Number helps the bootloader scan backwards to find the data */
#define FOOTER_MAGIC 0x454E4421  // ASCII "END!"

typedef struct {
    uint8_t  signature[64]; // ECDSA Signature (r + s)
    uint32_t version;       // Firmware Version
    uint32_t image_size;    // Size of the CODE only
    uint32_t magic;         // Fixed marker (0x454E4421)
} Firmware_Footer_t;

#endif

#endif /* INC_FIRMWARE_FOOTER_H_ */
