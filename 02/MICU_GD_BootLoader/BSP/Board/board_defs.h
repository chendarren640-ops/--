/* SPDX-License-Identifier: MIT */

/**
 * @file    board_defs.h
 * @brief   BootLoader board-level BSP definitions for GD32F470VET6
 *
 * Defines GPIO, USART, and DMA hardware mappings for the RS-485
 * debug UART, plus the associated initialization and control
 * function prototypes.
 */

#pragma once

#include "gd32f4xx.h"
#include "tick_timer.h"

#include "rs485_phy.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

/* ---- RS-485 debug UART: USART1 TX/RX = PA2/PA3, direction = PA1 ---- */

#define RS485_DIR_PORT              GPIOA
#define RS485_DIR_PORT_RCU          RCU_GPIOA
#define RS485_DIR_PIN               GPIO_PIN_1
#define RS485_DIR(x)                do { if(x) GPIO_BOP(RS485_DIR_PORT) = RS485_DIR_PIN; else GPIO_BC(RS485_DIR_PORT) = RS485_DIR_PIN; } while(0)

#define DBG_UART                    USART1
#define DBG_UART_RCU                RCU_USART1
#define DBG_UART_AF                 GPIO_AF_7
#define DBG_UART_PORT               GPIOA
#define DBG_UART_PORT_RCU           RCU_GPIOA
#define DBG_UART_TX_PIN             GPIO_PIN_2
#define DBG_UART_RX_PIN             GPIO_PIN_3

#define DBG_UART_RDATA_ADDRESS      ((uint32_t)(&USART_DATA(DBG_UART)))
#define DBG_UART_RXBUF_SIZE         1024U
#define DBG_UART_RX_DMA             DMA0
#define DBG_UART_RX_DMA_RCU         RCU_DMA0
#define DBG_UART_RX_DMA_CH          DMA_CH5
#define DBG_UART_RX_DMA_SUBPERI     DMA_SUBPERI4

/* ---- Function prototypes ---- */

/**
 * @brief  Initialize the debug USART peripheral with DMA reception
 */
void HalUart_Setup(void);

/**
 * @brief  Change the debug USART baud rate at runtime
 *
 * @param[in] baudrate  target baud rate value
 */
void HalUart_SetBaud(uint32_t baudrate);

/**
 * @brief  Return the current DMA receive byte count
 *
 * @return  number of bytes received by the DMA channel
 */
uint32_t HalUart_DmaRxPos(void);

/******************************************************************************/

#ifdef __cplusplus
}
#endif
