/*
 * BL_Functions.h
 *
 *  Created on: 27 Ara 2025
 *      Author: Oguzm
 */

#ifndef INC_BL_FUNCTIONS_H_
#define INC_BL_FUNCTIONS_H_

#include "mem_layout.h"
#include "stm32f4xx_hal.h"
#include "bootloader_interface.h"
#include <string.h> // For memcpy

typedef enum {
    TERM_NONE = 0,
    TERM_AUTO,
    TERM_BOOT,
    TERM_UPDATE,
    TERM_ROLLBACK,
    TERM_INFO
} term_cmd_t;

// Pointer to the config sector in Flash
#define CONFIG_FLASH_PTR ((BootConfig_t *)CONFIG_SECTOR_ADDR)


uint8_t BL_ReadConfig(BootConfig_t *cfg);
uint8_t BL_WriteFromPython(void);
void BL_PrintInfo(BootConfig_t *cfg);
term_cmd_t BL_ReadTerminalCommand(uint32_t timeout_ms);

void BL_Swap_NoBuffer(void);


#endif /* INC_BL_FUNCTIONS_H_ */

