/*
 * BL_Functions.c
 *
 *  Created on: 22 Ara 2025
 *      Author: Oguzm
 */

#include "main.h"
#include "BL_Functions.h"
#include "firmware_footer.h"

#ifndef SRC_BL_FUNCTIONS_C_
#define SRC_BL_FUNCTIONS_C_

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


    // 4) Hash plaintext image bytes [slot_start .. slot_start+image_size)

    /*uint8_t hash[32];
    if (!Crypto_SHA256_Flash((const uint8_t *)slot_start, f->image_size, hash)) {
        return BL_ERR_HASH_FAIL;
    }

    // 5) Verify signature over hash
    if (!Crypto_ECDSA_P256_VerifyHash(g_pubkey_xy, hash, f->signature)) {
        return BL_ERR_SIG_FAIL;
    }
*/

    return BL_OK;
}

static uint32_t Find_Footer(uint32_t slot_start, uint32_t slot_size)
{
//    if (slot_size < sizeof(fw_footer_t)) return 0;
//
//    uint32_t footer_addr = slot_start + slot_size - sizeof(fw_footer_t);
//    if (footer_addr < slot_start) return 0; // overflow/wrap protection
//
//    const fw_footer_t *f = (const fw_footer_t *)footer_addr;
//
//    if (f->magic != FOOTER_MAGIC) return 0;
//    if (f->image_size == 0) return 0;
//    if (f->image_size & 0x3u) return 0; // word aligned
//
//    uint32_t image_end = slot_start + f->image_size;
//    if (image_end > footer_addr) return 0; // must not overlap footer
//
//    return footer_addr;

	// 1. Basic sanity checks on input
	if (slot_size < sizeof(fw_footer_t)) return 0;

	uint32_t slot_end = slot_start + slot_size;

	// 2. Scan backwards for the Magic Number
	// We step by 4 bytes because flash writes and ARM instructions are typically word-aligned.
	for (uint32_t addr = slot_end - 4; addr >= slot_start + sizeof(fw_footer_t); addr -= 4)
	{
		// --- CHECK 1: Magic Number ---
		if (*(uint32_t*)addr == FOOTER_MAGIC)
		{
			// Calculate where the footer would start if this is the magic number
			// Struct: [Signature (64)] [Version (4)] [Size (4)] [Magic (4)]
			// Footer Start = Magic_Address - (Sizeof_Struct - Sizeof_Magic)
			uint32_t footer_start = addr - (sizeof(fw_footer_t) - 4);
			const fw_footer_t *f = (const fw_footer_t *)footer_start;

			// --- CHECK 2: Version Sanity ---
			// Version 0 or 0xFFFFFFFF (erased flash) is likely invalid
			if (f->version == 0 || f->version == 0xFFFFFFFF) continue;

			// --- CHECK 3: Image Size Alignment ---
			// ARM Cortex-M vectors and flash programming are word-aligned.
			// If the size isn't a multiple of 4, it's likely garbage data.
			if ((f->image_size % 4) != 0) continue;

			// --- CHECK 4: Image Size Bounds ---
			// The image cannot be size 0, and it cannot be larger than the slot.
			if (f->image_size == 0 || f->image_size > slot_size) continue;

			// --- CHECK 5: Geometric Consistency ---
			// This is the most important check.
			// The footer MUST be located exactly at: [Slot_Start + Image_Size]
			// If the math doesn't add up, this is just a random "END!" string in the binary code.
			uint32_t expected_footer_addr = slot_start + f->image_size;

			if (expected_footer_addr == footer_start)
			{
				return footer_start; // All checks passed!
			}
		}
	}

	return 0; // No valid footer found

}

// STUB 1: Pretend to hash the flash memory
// Returns 1 (true) to simulate success
int Crypto_SHA256_Flash(const uint8_t *addr, uint32_t size, uint8_t *digest)
{
    // TODO: Later, implement the loop using tc_sha256_init/update/final
    // For now, do nothing and return success
    return 1;
}

// STUB 2: Pretend to verify the signature
// Returns 1 (true) to simulate a "valid" signature
int Crypto_ECDSA_P256_VerifyHash(const uint8_t *pubkey, const uint8_t *hash, const uint8_t *sig)
{
    // TODO: Later, implement using tc_ecdsa_verify
    // For now, return 1 so the bootloader thinks the image is valid
    return 1;
}



#endif /* SRC_BL_FUNCTIONS_C_ */
