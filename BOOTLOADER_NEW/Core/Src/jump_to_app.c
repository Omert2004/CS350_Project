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

    if ((app_stack_addr & 0x20000000) != 0x20000000) {
        return;
    }

    // 2. DISABLE MPU & CACHE
    HAL_MPU_Disable();


    // 3. DISABLE SYSTICK (Crucial!)
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    // 5. DE-INIT
    HAL_DeInit();

    // 6. DISABLE INTERRUPTS
    __disable_irq();

    // 7. CLEAR PENDING INTERRUPTS
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    // 8. RELOCATE VECTOR TABLE
    SCB->VTOR = app_addr;

    // 9. JUMP
    __set_MSP(app_stack_addr);
    void (*pJump)(void) = (void (*)(void))app_reset_handler;
    pJump();
}
