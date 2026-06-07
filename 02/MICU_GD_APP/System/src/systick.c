/* SPDX-License-Identifier: MIT */

/**
 * @file    systick.c
 * @brief   SysTick timer driver for 1 kHz system tick and millisecond delay.
 */

#include "gd32f4xx.h"
#include "tick_timer.h"

/** @brief Millisecond delay counter, decremented in SysTick ISR. */
volatile static uint32_t m_TickDelay;

/**
 * @brief   Configure the SysTick timer for 1000 Hz interrupts.
 */
void Tick_Init(void)
{
    /* Setup systick timer for 1000 Hz interrupts */
    if (SysTick_Config(SystemCoreClock / 1000U))
    {
        /* Capture error */
        while (1)
        {
        }
    }
    /* Configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

/**
 * @brief   Blocking delay in milliseconds.
 * @param   count  Delay count in milliseconds.
 */
void Tick_DelayMs(uint32_t count)
{
    m_TickDelay = count;

    while (0U != m_TickDelay)
    {
    }
}

/**
 * @brief   Decrement the delay counter. Called from SysTick_Handler.
 */
void Tick_Decrement(void)
{
    if (0U != m_TickDelay)
    {
        m_TickDelay--;
    }
}
