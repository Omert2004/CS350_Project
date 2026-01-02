/*
 * tiny_printf.h
 *
 *  Created on: 22 Ara 2025
 *      Author: Oguzm
 */

#ifndef INC_TINY_PRINTF_H_
#define INC_TINY_PRINTF_H_

#include <stdarg.h>
#include <stdint.h>

// Initialize with the UART handle
void tfp_init(void* handle);

// The lightweight printf function
void tfp_printf(const char *fmt, ...);

// Macro to replace standard printf calls automatically
#define printf tfp_printf


#endif /* INC_TINY_PRINTF_H_ */
