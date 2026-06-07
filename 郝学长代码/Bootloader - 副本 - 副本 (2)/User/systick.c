/*!
    \file    systick.c
    \brief   SysTick 配置 - Bootloader 专用
*/

#include "gd32f4xx.h"
#include "systick.h"

volatile static uint32_t delay;
volatile uint32_t g_boot_tick_ms = 0;

void systick_config(void)
{
    if (SysTick_Config(SystemCoreClock / 1000U)){
        while (1){
        }
    }
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

void delay_1ms(uint32_t count)
{
    delay = count;
    while(0U != delay){
    }
}

void delay_decrement(void)
{
    if (0U != delay){
        delay--;
    }
}

void SysTick_Handler(void)
{
    g_boot_tick_ms++;
    delay_decrement();
}
