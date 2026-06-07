/* SPDX-License-Identifier: MIT */

/**
 * @file    gd32f4xx_it.c
 * @brief   Cortex-M4 exception and interrupt handlers for BootLoader
 *
 * Fault handlers trap unrecoverable exceptions in infinite loops.
 * The SysTick handler drives the non-blocking millisecond delay counter.
 */

#include "isr_handlers.h"
#include "main_defs.h"
#include "tick_timer.h"

/** @brief NMI exception handler — traps in infinite loop */
void NMI_Handler(void)
{
    while(1)
    {
    }
}

/** @brief HardFault exception handler — traps in infinite loop */
void HardFault_Handler(void)
{
    while(1)
    {
    }
}

/** @brief MemManage exception handler — traps in infinite loop */
void MemManage_Handler(void)
{
    while(1)
    {
    }
}

/** @brief BusFault exception handler — traps in infinite loop */
void BusFault_Handler(void)
{
    while(1)
    {
    }
}

/** @brief UsageFault exception handler — traps in infinite loop */
void UsageFault_Handler(void)
{
    while(1)
    {
    }
}

/** @brief SVC exception handler — traps in infinite loop */
void SVC_Handler(void)
{
    while(1)
    {
    }
}

/** @brief DebugMon exception handler — traps in infinite loop */
void DebugMon_Handler(void)
{
    while(1)
    {
    }
}

/** @brief PendSV exception handler — traps in infinite loop */
void PendSV_Handler(void)
{
    while(1)
    {
    }
}

/**
 * @brief  SysTick interrupt handler, fires every 1 ms
 *
 * Decrements the shared millisecond delay counter used by Tick_DelayMs().
 */
void SysTick_Handler(void)
{
    Tick_Decrement();
}
