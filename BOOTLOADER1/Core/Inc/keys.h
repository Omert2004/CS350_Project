/*
 * keys.h
 *
 *  Created on: 30 Ara 2025
 *      Author: Oguzm
 */

#ifndef INC_KEYS_H_
#define INC_KEYS_H_

#include <stdint.h>

// Use 'extern' to declare the variables without defining them
extern const uint8_t AES_SECRET_KEY[16];
extern const uint8_t ECDSA_public_key_xy[64]; // Adjust size as per your array

#endif /* INC_KEYS_H_ */
