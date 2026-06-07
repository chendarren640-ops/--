#include "boot_jump.h"

uint8_t BootJump_IsAppValid(void)
{
    return BSP_FLASH_IsAppValid(APP_START_ADDR);
}

void BootJump_JumpToApp(void)
{
    uint32_t app_stack;
    uint32_t app_entry;
    iap_function_t jump_to_app;
    uint32_t i;

    if(!BSP_FLASH_IsAppValid(APP_START_ADDR))
    {
        return;
    }

    app_stack = *(__IO uint32_t *)APP_START_ADDR;
    app_entry = *(__IO uint32_t *)(APP_START_ADDR + 4U);

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    for(i = 0; i < 8U; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }

    SCB->VTOR = APP_START_ADDR;

    __set_MSP(app_stack);

    jump_to_app = (iap_function_t)app_entry;

    jump_to_app();
}

