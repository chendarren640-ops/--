/* SPDX-License-Identifier: MIT */

/**
 * @file    rs485_phy.c
 * @brief   RS485 physical-layer driver implementation.
 *          Controls half-duplex direction pin and delegates UART setup
 *          to the common HAL UART initialiser.
 */

#include "rs485_phy.h"
#include "board_defs.h"

/** Current RS485 baud rate (default 19200). */
static uint32_t m_BaudRate = 19200U;

/**
 * @brief  Open the RS485 PHY at the given baud rate.
 * @param  baudrate  Desired baud rate.
 */
void Rs485Phy_Open(uint32_t baudrate)
{
    m_BaudRate = baudrate;
    HalUart_Setup();
}

/**
 * @brief  Change the RS485 baud rate at runtime.
 * @param  baudrate  New baud rate.
 */
void Rs485Phy_SetRate(uint32_t baudrate)
{
    m_BaudRate = baudrate;

    usart_disable(PROTO_UART);
    usart_baudrate_set(PROTO_UART, baudrate);
    usart_enable(PROTO_UART);
}

/**
 * @brief  Transmit a byte buffer over RS485 (half-duplex).
 * @param  data  Pointer to the buffer to send.
 * @param  len   Number of bytes to send.
 */
void Rs485Phy_Transmit(const uint8_t *data, size_t len)
{
    size_t i;

    if ((data == NULL) || (len == 0U))
    {
        return;
    }

    RS485_DIR(1);
    for (i = 0U; i < len; i++)
    {
        usart_data_transmit(PROTO_UART, data[i]);
        while (RESET == usart_flag_get(PROTO_UART, USART_FLAG_TBE))
        {
        }
    }
    while (RESET == usart_flag_get(PROTO_UART, USART_FLAG_TC))
    {
    }
    RS485_DIR(0);
}

/**
 * @brief  Return the currently configured baud rate.
 * @return Current baud rate in bits per second.
 */
uint32_t Rs485Phy_GetRate(void)
{
    return m_BaudRate;
}
