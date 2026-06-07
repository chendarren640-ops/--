/* SPDX-License-Identifier: MIT */

/**
 * @file    usart_app.h
 * @brief   Public API for the application-layer UART driver.
 */

#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Public variables (used by ISR and OTA modules) ---- */

/** Transmit byte counter (loop index in Uart_Printf). */
extern __IO uint16_t g_TxCount;

/** PROTO_UART DMA RX complete flag, set by ISR. */
extern __IO uint8_t g_RxFlag;

/** Number of bytes received in the last PROTO_UART DMA transfer. */
extern __IO uint16_t g_UartDmaLen;

/** DMA receive buffer for the PROTO_UART. */
extern uint8_t g_UartDmaBuf[];

/** DBG_UART DMA RX complete flag, set by ISR. */
extern __IO uint8_t g_DebugRxFlag;

/** Number of bytes received in the last DBG_UART DMA transfer. */
extern __IO uint16_t g_DebugDmaLen;

/** DMA receive buffer for the DBG_UART. */
extern uint8_t g_DebugDmaBuf[];

/** Circular DMA receive buffer for the PROTO_UART (defined in BSP/ISR). */
extern uint8_t g_RxBuf[];

/* ---- Public functions ---- */

/** Formatted transmit over a UART peripheral. */
int Uart_Printf(uint32_t usart_periph, const char *format, ...);

/** UART periodic task (called from scheduler every 5 ms). */
void Uart_Process(void);

/** Reset OTA UART reception state (DMA position, buffers, OTA parser). */
void OtaRx_Reset(void);

/** Debug UART DMA reception callback. */
void DebugUart_RxCallback(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif
