/* SPDX-License-Identifier: MIT */

/**
 * @file    usart_app.c
 * @brief   Formatted USART output with RS-485 direction control
 *
 * Provides a printf-style wrapper that transmits a formatted string
 * over the selected USART peripheral. When the peripheral matches
 * DBG_UART the RS-485 direction pin is toggled before and after
 * transmission.
 */

#include "board_defs.h"

/** @brief Running transmit byte counter */
volatile uint16_t g_TxCount;

/**
 * @brief  Transmit a formatted string over a USART peripheral
 *
 * Formats the variadic arguments into an internal buffer, optionally
 * asserts the RS-485 driver-enable line, sends every byte and waits
 * for TX completion before de-asserting the line.
 *
 * @param[in] usart_periph  USART base address (USARTx)
 * @param[in] format        printf-style format string
 * @param[in] ...           variadic arguments referenced by format
 * @return                  number of characters transmitted
 */
int Uart_Printf(uint32_t usart_periph, const char *format, ...)
{
    char buffer[512];
    va_list arg;
    int len;

    va_start(arg, format);
    len = vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);

    if(usart_periph == DBG_UART)
    {
        RS485_DIR(1);
    }

    for(g_TxCount = 0; g_TxCount < (uint16_t)len; g_TxCount++)
    {
        usart_data_transmit(usart_periph, (uint8_t)buffer[g_TxCount]);
        while(RESET == usart_flag_get(usart_periph, USART_FLAG_TBE))
        {
        }
    }

    if(usart_periph == DBG_UART)
    {
        while(RESET == usart_flag_get(usart_periph, USART_FLAG_TC))
        {
        }
        RS485_DIR(0);
    }

    return len;
}
