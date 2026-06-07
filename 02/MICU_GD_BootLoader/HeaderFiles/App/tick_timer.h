/* SPDX-License-Identifier: MIT */

/**
 * @file    tick_timer.h
 * @brief   SysTick millisecond timer declarations
 */

#pragma once

#include <stdint.h>

/**
 * @brief  Initialize the SysTick timer for 1000 Hz interrupts
 */
void Tick_Init(void);

/**
 * @brief  Blocking millisecond delay
 *
 * @param[in] count  delay duration in milliseconds
 */
void Tick_DelayMs(uint32_t count);

/**
 * @brief  Decrement the delay counter (called from SysTick ISR)
 */
void Tick_Decrement(void);
