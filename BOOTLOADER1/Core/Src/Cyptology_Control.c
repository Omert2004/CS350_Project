/*
 * Cyptology_Control.c
 *
 *  Created on: 30 Ara 2025
 *      Author: Oguzm
 */
#include "Cryptology_Control.h"
#include "sha256.h"
#include "ecc_dsa.h"
#include "firmware_footer.h"
#include <string.h>
#include "mem_layout.h"

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
            // We found the Magic at 'addr'.
            // The footer starts before the magic.
            // Struct layout: [Ver][Size][Sig][Magic]
            // Footer Start = Magic_Address - offsetof(magic)
            // But simpler: Footer Start = Magic_Address - (TotalSize - 4)
            uint32_t footer_start = addr - (sizeof(fw_footer_t) - 4);

            // Double check safety
            if (footer_start < slot_start) continue;

            fw_footer_t *f = (fw_footer_t *)footer_start;

            // Verify Consistency: Firmware Size + Footer Size should land us here
            uint32_t calculated_end = slot_start + f->size;

            if (calculated_end == footer_start) {
                return footer_start; // Found valid footer!
            }
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

    // 2. Hash
    struct tc_sha256_state_struct s;
    uint8_t digest[32];

    (void)tc_sha256_init(&s);
    tc_sha256_update(&s, (uint8_t*)(APP_DOWNLOAD_START_ADDR + 4), data_len);
    (void)tc_sha256_final(digest, &s);

    // 3. Verify
    int result = uECC_verify(
        ECDSA_public_key_xy,
        digest,
        32,
        footer->signature,
        uECC_secp256r1()
    );

    return result;
}



#include "tinycrypt/sha256.h" // Ensure this include is present

int Verify_Signature(uint8_t* pData, uint32_t Size, uint8_t* pSignature)
{
    uint8_t hash[32];
    struct tc_sha256_state_struct s;

    // 1. Initialize SHA-256
    tc_sha256_init(&s);

    // 2. Hash the Payload (IV + Encrypted Data)
    tc_sha256_update(&s, pData, Size);

    // 3. Finalize to get the 32-byte digest
    tc_sha256_final(hash, &s);

    // 4. Verify the signature against the HASH, not pData
    int result = uECC_verify(public_key, hash, pSignature, uECC_secp256r1());

    return result;
}
