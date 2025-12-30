/*
 * Cyptology_Control.c
 *
 * Created on: 30 Ara 2025
 * Author: Oguzm
 */
#include "Cryptology_Control.h"
#include "sha256.h"
#include "ecc_dsa.h"
#include "firmware_footer.h"
#include <string.h>
#include "mem_layout.h"
#include "keys.h"

extern const uint8_t ECDSA_public_key_xy[];

/* Scans the slot backwards to find the footer magic number */
uint32_t Find_Footer_Address(uint32_t slot_start, uint32_t slot_size)
{
    uint32_t slot_end = slot_start + slot_size;

    // Scan backwards from end of slot, 4 bytes at a time
    for (uint32_t addr = slot_end - 4; addr >= slot_start; addr -= 4)
    {
        // Check for Magic Number
        if (*(uint32_t*)addr == FOOTER_MAGIC)
        {
            // Found Magic! The footer ends here.
            // Struct: [Ver][Size][Sig][Magic]
            // We need to back up by (sizeof(fw_footer_t) - 4) to find the start
            uint32_t footer_start = addr - (sizeof(fw_footer_t) - 4);

            // Safety check
            if (footer_start < slot_start) continue;

            // Optional: Verify consistency (e.g. Size + FooterAddr match)
            // But for now, returning the address is sufficient
            return footer_start;
        }
    }
    return 0; // Not found
}

/* Updated Validation Function */
int Firmware_Is_Valid(uint32_t start_addr, uint32_t slot_size)
{
    // 1. Find the footer dynamically
    uint32_t footer_addr = Find_Footer_Address(start_addr, slot_size);

    if (footer_addr == 0) {
        return 0; // No footer found
    }

    fw_footer_t *footer = (fw_footer_t *)footer_addr;

    // 2. Hash the Payload
    struct tc_sha256_state_struct s;
    uint8_t digest[32];

    (void)tc_sha256_init(&s);

    // FIXED: Use footer->size (from firmware_footer.h)
    // This size represents the encrypted payload size (IV + Data)
    tc_sha256_update(&s, (uint8_t*)start_addr, footer->size);

    (void)tc_sha256_final(digest, &s);

    // 3. Verify Signature
    int result = uECC_verify(
        ECDSA_public_key_xy,
        digest,
        32,
        footer->signature,
        uECC_secp256r1()
    );

    return result; // 1 = Valid, 0 = Invalid
}

// Helper if you need to verify a raw buffer manually
int Verify_Signature(uint8_t* pData, uint32_t Size, uint8_t* pSignature)
{
    uint8_t hash[32];
    struct tc_sha256_state_struct s;

    tc_sha256_init(&s);
    tc_sha256_update(&s, pData, Size);
    tc_sha256_final(hash, &s);

    int result = uECC_verify(
        ECDSA_public_key_xy,
        hash,
        32,
        pSignature,
        uECC_secp256r1()
    );

    return result;
}
