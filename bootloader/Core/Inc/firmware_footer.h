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
    BL_OK = 0,                     // Success

    /* Slot / Input Errors */
    BL_ERR_INVALID_SLOT = -1,
    BL_ERR_SLOT_OUT_OF_RANGE = -2,

    /* Footer Errors */
    BL_ERR_FOOTER_NOT_FOUND = -10,
    BL_ERR_FOOTER_MAGIC_MISMATCH = -11,
    BL_ERR_FOOTER_VERSION_UNSUPPORTED = -12,
    BL_ERR_FOOTER_SIZE_INVALID = -13,
    BL_ERR_FOOTER_STREAM_SIZE_INVALID = -14,

    /* Signature / Crypto Errors */
    BL_ERR_SIGNATURE_INVALID = -20,
    BL_ERR_HASH_MISMATCH = -21,
    BL_ERR_PUBLIC_KEY_INVALID = -22,

    /* AES / Decryption Errors */
    BL_ERR_AES_DECRYPT_FAIL = -30,
    BL_ERR_AES_INVALID_CHUNK_SIZE = -31,
    BL_ERR_AES_IV_INVALID = -32,

    /* LZ4 Decompression Errors */
    BL_ERR_LZ4_FAIL = -40,
    BL_ERR_LZ4_OUTPUT_OVERFLOW = -41,

    /* Flash Write / Erase Errors */
    BL_ERR_FLASH_UNLOCK_FAIL = -50,
    BL_ERR_FLASH_ERASE_FAIL = -51,
    BL_ERR_FLASH_PROGRAM_FAIL = -52,
    BL_ERR_FLASH_ALIGN_ERROR = -53,

    /* Generic Error */
    BL_ERR_UNKNOWN = -100
} BL_Status_t;


typedef struct __attribute__((packed)) {  //attr packed -> don t align the size if not meet
    uint32_t magic;            // 0x454E4421
    uint32_t footer_version;   // format version

    uint32_t image_version;    // anti-rollback (monotonic counter)
    uint32_t image_size;       // plaintext firmware size

    uint32_t stream_size;      // encrypted+compressed stream size
    uint32_t load_address;     // where decrypted app will be written
    uint32_t entry_point;      // app's reset handler address

    uint32_t flags;            // bits 0–3: compression, bits 4–7: encryption

    uint8_t  hash[32];         // SHA-256 of plaintext firmware
    uint8_t  signature[64];    // ECDSA P-256 signature over hash
} fw_footer_t;  // Total size = 4+4+4+4+4+4+4+4 + 32 + 64 = 128 bytes

#endif

#endif /* INC_FIRMWARE_FOOTER_H_ */
