/* SPDX-License-Identifier: MIT */

/**
 * @file    isr_handlers.h
 * @brief   Cortex-M4 fault and interrupt handler declarations
 */

#pragma once

#include "gd32f4xx.h"

/* ---- Fault and exception handler declarations ---- */

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);

/* ---- System timer handler ---- */

void SysTick_Handler(void);
