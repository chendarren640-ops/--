/**
 * @file    gd32f4xx_it.c
 * @brief   CIMC 2026 中断服务函数
 */

#include "gd32f4xx_it.h"
#include "HeaderFiles.h"

extern volatile uint32_t g_sys_tick_ms;

void NMI_Handler(void)       { while(1); }
void MemManage_Handler(void) { while(1); }
void BusFault_Handler(void)  { while(1); }
void UsageFault_Handler(void){ while(1); }
void SVC_Handler(void)       {}
void DebugMon_Handler(void)  {}
void PendSV_Handler(void)    {}

/**
 * @brief SysTick 1ms 中断
 */
void SysTick_Handler(void)
{
    g_sys_tick_ms++;
    delay_decrement();
}
