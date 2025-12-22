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

// stattes
typedef enum {
    BL_OK = 0,			//success
    BL_ERR_FOOTER_NOT_FOUND,
    BL_ERR_FOOTER_BAD,
    BL_ERR_IMAGE_SIZE_BAD,
    BL_ERR_IMAGE_RANGE_BAD,
    BL_ERR_VECTOR_BAD,
    BL_ERR_HASH_FAIL,
    BL_ERR_SIG_FAIL,
} BL_Status_t;



typedef struct /* __attribute__((packed)) */ {  //attr packed -> don t align the size if not meet
	uint8_t  signature[64];
	uint32_t version;
	uint32_t image_size;
	uint32_t magic;
} fw_footer_t;  // 1 x 64 + 4 + 4 + 4  => 76 byte


#endif

#endif /* INC_FIRMWARE_FOOTER_H_ */
