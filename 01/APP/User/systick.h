/**
 * 2026 CIMC APP — SysTick 头文件
 */

#ifndef __SYSTICK_H
#define __SYSTICK_H

#include "main.h"

extern volatile uint32_t g_sys_tick;

void     systick_config(void);
void     SysTick_Handler_Proc(void);
void     delay_1ms(uint32_t count);
uint32_t millis(void);

#endif
