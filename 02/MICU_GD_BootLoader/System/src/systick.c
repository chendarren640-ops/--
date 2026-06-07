/* SPDX-License-Identifier: MIT */

/**
 * @file    systick.c
 * @brief   SysTick millisecond timer driver for BootLoader
 *
 * Configures the Cortex-M4 SysTick to generate interrupts at 1000 Hz.
 * Provides blocking millisecond delay and the ISR-callback decrement.
 */

#include "gd32f4xx.h"
#include "tick_timer.h"

/** @brief Shared delay counter, decremented in SysTick ISR */
static volatile uint32_t m_TickDelay;

/**
 * @brief  Initialize the SysTick timer for 1000 Hz interrupts
 *
 * Calls SysTick_Config() with SystemCoreClock / 1000. On error the
 * function traps in an infinite loop.
 */
void Tick_Init(void)
{
    if(SysTick_Config(SystemCoreClock / 1000U))
    {
        while(1)
        {
        }
    }
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

/**
 * @brief  Blocking millisecond delay
 *
 * Sets the shared counter and waits until the SysTick ISR has
 * decremented it to zero.
 *
 * @param[in] count  delay duration in milliseconds
 */
void Tick_DelayMs(uint32_t count)
{
    m_TickDelay = count;

    while(0U != m_TickDelay)
    {
    }
}

/**
 * @brief  Decrement the delay counter (called from SysTick ISR)
 *
 * If the counter is non-zero it is decremented by one.
 */
void Tick_Decrement(void)
{
    if(0U != m_TickDelay)
    {
        m_TickDelay--;
    }
}
