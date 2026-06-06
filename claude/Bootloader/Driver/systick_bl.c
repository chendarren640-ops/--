/**
 * Bootloader — SysTick 1ms 定时
 */
#include "systick_bl.h"

volatile uint32_t bl_tick = 0;

void systick_config(void)
{
    if (SysTick_Config(SystemCoreClock / 1000U))
        while (1);
    NVIC_SetPriority(SysTick_IRQn, 0x00);
}

void delay_1ms(uint32_t count)
{
    uint32_t start = bl_tick;
    while ((bl_tick - start) < count);
}

void SysTick_Handler(void)
{
    bl_tick++;
}
