/* SPDX-License-Identifier: MIT */

/**
 * @file    isr_defs.h
 * @brief   Declarations for all interrupt service routines.
 */

#pragma once

#include "gd32f4xx.h"

/* Core exception handlers */
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);

/* UART ISR handlers (one enabled per board configuration by PROTO_UART_IRQN) */
void USART0_IRQHandler(void);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void UART3_IRQHandler(void);
void UART4_IRQHandler(void);
void USART5_IRQHandler(void);
void UART6_IRQHandler(void);
void UART7_IRQHandler(void);

/* External interrupt and wakeup handlers */
void EXTI0_IRQHandler(void);
void RTC_WKUP_IRQHandler(void);

/* SysTick handler */
void SysTick_Handler(void);
