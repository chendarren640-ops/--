/* SPDX-License-Identifier: MIT */

/**
 * @file    tick_timer.h
 * @brief   SysTick timer driver public interface.
 */

#pragma once

#include <stdint.h>

/** Configure the SysTick timer for 1000 Hz interrupts. */
void Tick_Init(void);

/** Blocking delay in milliseconds. */
void Tick_DelayMs(uint32_t count);

/** Decrement the delay counter, called from SysTick ISR. */
void Tick_Decrement(void);
