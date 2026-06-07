/* SPDX-License-Identifier: MIT */

/**
 * @file    rs485_phy.h
 * @brief   RS485 physical-layer driver — half-duplex USART with direction
 *          control for the GD32F470VET6 board.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief  Open the RS485 PHY at the given baud rate.
 * @param  baudrate  Desired baud rate (e.g. 19200, 115200).
 */
void Rs485Phy_Open(uint32_t baudrate);

/**
 * @brief  Change the RS485 baud rate at runtime.
 * @param  baudrate  New baud rate.
 */
void Rs485Phy_SetRate(uint32_t baudrate);

/**
 * @brief  Transmit a byte buffer over RS485 (half-duplex).
 * @param  data  Pointer to the buffer to send.
 * @param  len   Number of bytes to send.
 */
void Rs485Phy_Transmit(const uint8_t *data, size_t len);

/**
 * @brief  Return the currently configured baud rate.
 * @return Current baud rate in bits per second.
 */
uint32_t Rs485Phy_GetRate(void);

#ifdef __cplusplus
}
#endif
