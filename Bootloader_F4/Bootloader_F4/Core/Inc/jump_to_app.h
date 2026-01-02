/*
 * jump_to_app.h
 *
 *  Created on: 22 Ara 2025
 *      Author: Oguzm
 */

#ifndef INC_JUMP_TO_APP_H_
#define INC_JUMP_TO_APP_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define UART_TIMEOUT     HAL_MAX_DELAY
#define BL_CHUNK_SIZE    256
extern uint32_t firmware_size;
extern uint32_t startaddr;

typedef struct {
    uint32_t size;
    uint32_t crc;
} bl_header_t;

void Bootloader_Run(void);
void Bootloader_JumpToApp(void);

#endif /* INC_JUMP_TO_APP_H_ */
