/*
 * BL_Functions.h
 *
 *  Created on: 27 Ara 2025
 *      Author: Oguzm
 */

#ifndef INC_BL_FUNCTIONS_H_
#define INC_BL_FUNCTIONS_H_

#include "mem_layout.h"
#include "stm32f7xx_hal.h"
#include <string.h> // For memcpy

// Pointer to the config sector in Flash
#define CONFIG_FLASH_PTR ((BootConfig_t *)CONFIG_SECTOR_ADDR)

uint8_t BL_ReadConfig(BootConfig_t *cfg);
uint8_t BL_WriteConfig(BootConfig_t *cfg);


void BL_Swap_NoBuffer(void);

#endif /* INC_BL_FUNCTIONS_H_ */

