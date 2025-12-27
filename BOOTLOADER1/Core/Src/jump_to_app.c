/*
 * jump_to_app.c
 *
 *  Created on: 22 Ara 2025
 *      Author: Oguzm
 */


#include "main.h"
#include "jump_to_app.h"
#include "mem_layout.h"
void Bootloader_JumpToApp(void) {
    uint32_t app_addr = APP_ACTIVE_START_ADDR;
    uint32_t app_stack_addr = *(volatile uint32_t*)app_addr;
    uint32_t app_reset_handler = *(volatile uint32_t*)(app_addr + 4);

    //volatile uint32_t ctrl = __get_CONTROL();

    if ((app_stack_addr & 0x20000000) != 0x20000000) {
        return;
    }

    // 1. DISABLE MPU (The ONLY new thing you need)
    HAL_MPU_Disable();

    // 2. DISABLE SYSTICK
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    // 3. DE-INIT PERIPHERALS
    // This will not crash now because we removed the printf in main
    //HAL_DeInit();

    // 4. DISABLE INTERRUPTS
    __disable_irq();


    HAL_DeInit();

    // 5. CLEAR PENDING INTERRUPTS
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    // 6. RELOCATE VECTOR TABLE
    SCB->VTOR = app_addr;

    // 7. JUMP
    __set_MSP(app_stack_addr);
    void (*pJump)(void) = (void (*)(void))app_reset_handler;
    pJump();
}

