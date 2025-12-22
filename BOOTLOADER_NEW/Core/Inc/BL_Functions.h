/*
 * BL_Functions.h
 *
 *  Created on: 22 Ara 2025
 *      Author: Oguzm
 */

#ifndef INC_BL_FUNCTIONS_H_
#define INC_BL_FUNCTIONS_H_

#include "firmware_footer.h"
#include <stdint.h>

BL_Status_t Bootloader_InternalVerify(uint32_t slot_start, uint32_t slot_size);
int Crypto_SHA256_Flash(const uint8_t *addr, uint32_t size, uint8_t *digest);
int Crypto_ECDSA_P256_VerifyHash(const uint8_t *pubkey, const uint8_t *hash, const uint8_t *sig);

#endif /* INC_BL_FUNCTIONS_H_ */
