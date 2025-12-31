/**
 * @file    Cryptology_Control.c
 * @brief   Firmware Verification and Footer Management.
 * @details Implements the logic to locate the firmware footer in Flash,
 * hash the payload (SHA-256), and verify the ECDSA signature.
 */

#include "Cryptology_Control.h"
#include "sha256.h"
#include "ecc_dsa.h"
#include "firmware_footer.h"
#include <string.h>
#include "mem_layout.h"
#include "keys.h"

extern const uint8_t ECDSA_public_key_xy[];

/**
 * @brief  Scans the Flash slot backwards to locate the Firmware Footer.
 * @details Looks for the FOOTER_MAGIC marker starting from the end of the slot.
 * @param  slot_start Start address of the Flash sector/slot.
 * @param  slot_size  Size of the slot in bytes.
 * @retval Address of the fw_footer_t structure, or 0 if not found.
 */
uint32_t Find_Footer_Address(uint32_t slot_start, uint32_t slot_size)
{
    uint32_t slot_end = slot_start + slot_size;

    // Scan backwards from end of slot, 4 bytes at a time
    for (uint32_t addr = slot_end - 4; addr >= slot_start; addr -= 4)
    {
        if (*(uint32_t*)addr == FOOTER_MAGIC)
        {
            uint32_t footer_start = addr - (sizeof(fw_footer_t) - 4);
            if (footer_start < slot_start) continue;
            return footer_start;
        }
    }
    return 0; // Not found
}

/**
 * @brief  Validates the integrity and authenticity of a firmware image.
 * @details Steps:
 * 1. Locate Footer.
 * 2. Calculate SHA-256 hash of the payload.
 * 3. Verify ECDSA signature using the stored Public Key.
 * @param  start_addr Start address of the image in Flash.
 * @param  slot_size  Maximum size of the slot.
 * @retval BL_OK on success, or specific error code (e.g., BL_ERR_SIG_FAIL) on failure.
 */
FW_Status_t Firmware_Is_Valid(uint32_t start_addr, uint32_t slot_size)
{
    // 1. Find the footer
    uint32_t footer_addr = Find_Footer_Address(start_addr, slot_size);

    if (footer_addr == 0) {
        return BL_ERR_FOOTER_NOT_FOUND;
    }

    fw_footer_t *footer = (fw_footer_t *)footer_addr;

    // Sanity check: Ensure payload size isn't larger than the slot
    if (footer->size > slot_size) {
        return BL_ERR_IMAGE_SIZE_BAD;
    }

    // 2. Hash the Payload
    struct tc_sha256_state_struct s;
    uint8_t digest[32];

    if (tc_sha256_init(&s) != 1) return BL_ERR_HASH_FAIL;

    // Hash the exact size specified in the footer
    tc_sha256_update(&s, (uint8_t*)start_addr, footer->size);

    if (tc_sha256_final(digest, &s) != 1) return BL_ERR_HASH_FAIL;

    // 3. Verify Signature
    int verify_result = uECC_verify(
        ECDSA_public_key_xy,
        digest,
        32,
        footer->signature,
        uECC_secp256r1()
    );

    if (verify_result == 1) {
        return BL_OK; // Success!
    } else {
        return BL_ERR_SIG_FAIL; // Cryptographic failure
    }
}
