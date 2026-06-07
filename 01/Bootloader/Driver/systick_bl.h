/**
 * Bootloader — SysTick 延时驱动 (精简版)
 */
#ifndef __SYSTICK_BL_H
#define __SYSTICK_BL_H

#include "../bootloader.h"

extern volatile uint32_t bl_tick;

void systick_config(void);
void delay_1ms(uint32_t count);

#endif
