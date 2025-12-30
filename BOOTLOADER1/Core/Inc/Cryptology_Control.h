/*
 * Cryptology_Control.h
 *
 *  Created on: 30 Ara 2025
 *      Author: Oguzm
 */



#ifndef INC_CRYPTOLOGY_CONTROL_H_
#define INC_CRYPTOLOGY_CONTROL_H_

#include <stdint.h>

uint32_t Find_Footer_Address(uint32_t slot_start, uint32_t slot_size);
int Firmware_Is_Valid(uint32_t start_addr, uint32_t size);

#endif /* INC_CRYPTOLOGY_CONTROL_H_ */
