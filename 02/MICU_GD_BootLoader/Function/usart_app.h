/* SPDX-License-Identifier: MIT */

/**
 * @file    usart_app.h
 * @brief   Formatted USART output declarations
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Running transmit byte counter */
extern volatile uint16_t g_TxCount;

/**
 * @brief  Transmit a formatted string over a USART peripheral
 *
 * @param[in] usart_periph  USART base address
 * @param[in] format        printf-style format string
 * @param[in] ...           variadic arguments
 * @return                  number of characters transmitted
 */
int Uart_Printf(uint32_t usart_periph, const char *format, ...);

#ifdef __cplusplus
}
#endif
