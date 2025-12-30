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
    (void)tc_sha256_update(&s, (const uint8_t *)start_addr, footer->size);
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


/*

BL_Status_t Bootloader_InternalVerify(uint32_t slot_start, uint32_t slot_size)
{
	// 1) Find footer (deterministic)
	uint32_t footer_addr = Find_Footer(slot_start, slot_size);
	if (footer_addr == 0) return BL_ERR_FOOTER_NOT_FOUND;

	const fw_footer_t *f = (const fw_footer_t *)footer_addr;

	// 2) Image range checks (again, explicit)
	uint32_t slot_end = slot_start + slot_size;
	uint32_t image_end = slot_start + f->image_size;

	if (slot_end < slot_start) return BL_ERR_IMAGE_RANGE_BAD;    // overflow protection
	if (image_end > footer_addr) return BL_ERR_IMAGE_RANGE_BAD;
	if (image_end > slot_end) return BL_ERR_IMAGE_RANGE_BAD;


	// 4) Hash plaintext image bytes [slot_start .. image_end)
	// Note: We hash ONLY the image, not the footer!
	uint8_t hash[32];
	if (!Crypto_SHA256_Flash((const uint8_t *)slot_start, f->image_size, hash)) {
		return BL_ERR_HASH_FAIL;
	}

	// 5) Verify signature over hash
	// We pass NULL for pubkey here because the function now looks up extern public_key_x/y internally
	if (!Crypto_ECDSA_P256_VerifyHash(NULL, hash, f->signature)) {
		return BL_ERR_SIG_FAIL;
	}

	return BL_OK;
}



 * Calculates SHA-256 of the Flash Memory area
 * Returns 1 on Success, 0 on Failure

int Crypto_SHA256_Flash(const uint8_t *addr, uint32_t size, uint8_t *digest)
{
	struct tc_sha256_state_struct s;
	(void)tc_sha256_init(&s);

	// Process in chunks to avoid blocking CPU for too long (optional but good practice)
	uint32_t remaining = size;
	const uint8_t *ptr = addr;

	while (remaining > 0) {
		uint32_t chunk = (remaining > 4096) ? 4096 : remaining;
		(void)tc_sha256_update(&s, ptr, chunk);
		ptr += chunk;
		remaining -= chunk;
	}

	(void)tc_sha256_final(digest, &s);
	return 1;
}

 * Verifies the ECDSA P-256 Signature
 * Returns 1 if Valid, 0 if Invalid

int Crypto_ECDSA_P256_VerifyHash(const uint8_t *pubkey, const uint8_t *hash, const uint8_t *sig)
{
	// TinyCrypt expects the public key as 64 raw bytes (X + Y)
	// If your keys.c has them separate, we combine them here:
	uint8_t pub_key_combined[64];
	memcpy(pub_key_combined, ECDSA_PUBLIC_KEY_X, 32);
	memcpy(&pub_key_combined[32], ECDSA_PUBLIC_KEY_Y, 32);

	// The signature in the footer is 64 bytes (R + S)
	// TinyCrypt verify function:
	// int uECC_verify(const uint8_t *public_key, const uint8_t *message_hash,
	//                 unsigned hash_size, const uint8_t *signature, uECC_Curve curve);

	int result = uECC_verify(pub_key_combined,
							 hash,
							 32, // SHA256 Hash size
							 sig,
							 uECC_secp256r1());

	return result; // uECC returns 1 on success
}
*/

