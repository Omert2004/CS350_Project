/*
 * BL_Functions.h
 *
 *  Created on: 27 Ara 2025
 *      Author: Oguzm
 */

#ifndef INC_BL_FUNCTIONS_H_
#define INC_BL_FUNCTIONS_H_

#include "bootloader_interface.h"
#include "stm32f7xx_hal.h"
#include <string.h> // For memcpy


uint8_t BL_ReadConfig(BootConfig_t *cfg);
uint8_t BL_WriteConfig(BootConfig_t *cfg);

void BL_Swap_NoBuffer(void);
void BL_Rollback(void);

#endif /* INC_BL_FUNCTIONS_H_ */

