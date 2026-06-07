/* SPDX-License-Identifier: MIT */

/**
 * @file    usart_app.c
 * @brief   Application-layer UART driver for the CIMC protocol and OTA
 *          firmware updates.
 *
 * Manages DMA-based reception on two UART peripherals:
 *   - PROTO_UART (RS-485):  CIMC ASCII protocol frames and OTA binary
 *     firmware frames.
 *   - DBG_UART:             Debug / console output and debug frame reception.
 *
 * ASCII frame parsing searches for the A5B6 header / B6A5 tail pattern
 * defined by the CIMC protocol and dispatches complete frames to the
 * application command handler.
 */

#include "board_defs.h"
#include "cimc_app.h"
#include "cimc_protocol_defs.h"
#include "ota_uart.h"

/** Transmit byte counter (used as loop index in Uart_Printf). */
__IO uint16_t g_TxCount = 0;

/** Flag set by the PROTO_UART DMA RX ISR when new data is available. */
__IO uint8_t g_RxFlag = 0;

/** Number of bytes received in the last PROTO_UART DMA transfer. */
__IO uint16_t g_UartDmaLen = 0;

/** DMA receive buffer for the PROTO_UART. */
uint8_t g_UartDmaBuf[OTA_UART_RXBUF_SIZE] = {0};

/** Flag set by the DBG_UART DMA RX ISR when new data is available. */
__IO uint8_t g_DebugRxFlag = 0;

/** Number of bytes received in the last DBG_UART DMA transfer. */
__IO uint16_t g_DebugDmaLen = 0;

/** DMA receive buffer for the DBG_UART. */
uint8_t g_DebugDmaBuf[DEBUG_UART_RXBUF_SIZE] = {0};

/** Circular DMA buffer for OTA UART raw bytes (defined in ISR / BSP layer). */
extern uint8_t rxbuffer[OTA_UART_RXBUF_SIZE];

/*
 * DMA writes OTA UART bytes into rxbuffer as a circular buffer. This cursor
 * tracks how far the application has already handed bytes to the OTA parser
 * and the ASCII frame parser.
 */
static uint32_t m_OtaDmaLastPos = 0U;

/** Accumulation buffer for an in-progress ASCII protocol frame. */
static char m_FrameBuf[CIMC_FRAME_MAX_ASCII_LEN];

/** Current length of the ASCII frame being accumulated. */
static uint16_t m_FrameLen = 0U;

/* ------------------------------------------------------------------ */
/*  ASCII frame parser helpers                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief Test whether a byte is a valid hexadecimal character.
 * @param ch  The byte to test.
 * @return    1 if ch is [0-9A-Fa-f], 0 otherwise.
 */
static uint8_t IsHexChar(uint8_t ch)
{
    return (((ch >= '0') && (ch <= '9')) ||
            ((ch >= 'A') && (ch <= 'F')) ||
            ((ch >= 'a') && (ch <= 'f')));
}

/**
 * @brief Check whether the accumulated ASCII frame ends with the CIMC
 *        frame tail marker "B6A5".
 * @return 1 if the last four bytes are 'B','6','A','5', 0 otherwise.
 */
static uint8_t ProtoFrame_HasTail(void)
{
    if (m_FrameLen < 4U)
    {
        return 0U;
    }

    return ((m_FrameBuf[m_FrameLen - 4U] == 'B') &&
            (m_FrameBuf[m_FrameLen - 3U] == '6') &&
            (m_FrameBuf[m_FrameLen - 2U] == 'A') &&
            (m_FrameBuf[m_FrameLen - 1U] == '5'));
}

/**
 * @brief Feed raw bytes into the ASCII protocol frame parser.
 *
 * The parser searches for the A5B6 header / B6A5 tail pattern.  Valid
 * hex characters between the header and tail are accumulated.  When a
 * complete frame is detected, it is dispatched to the application
 * command handler.
 *
 * @param data  Pointer to the raw byte buffer.
 * @param len   Number of bytes to process.
 */
static void ProtoFrame_FeedBytes(const uint8_t *data, uint32_t len)
{
    uint32_t i;

    for (i = 0U; i < len; i++)
    {
        uint8_t ch = data[i];

        if (!IsHexChar(ch))
        {
            m_FrameLen = 0U;
            continue;
        }

        if (m_FrameLen == 0U)
        {
            if (ch != 'A')
            {
                continue;
            }
            m_FrameBuf[m_FrameLen++] = (char)ch;
            continue;
        }

        if (m_FrameLen >= CIMC_FRAME_MAX_ASCII_LEN)
        {
            m_FrameLen = 0U;
            continue;
        }

        m_FrameBuf[m_FrameLen++] = (char)ch;

        if (m_FrameLen == 1U && m_FrameBuf[0] != 'A')
        {
            m_FrameLen = 0U;
        }
        else if (m_FrameLen == 2U && m_FrameBuf[1] != '5')
        {
            m_FrameLen = (ch == 'A') ? 1U : 0U;
            if (m_FrameLen == 1U)
            {
                m_FrameBuf[0] = 'A';
            }
        }
        else if (m_FrameLen == 3U && m_FrameBuf[2] != 'B')
        {
            m_FrameLen = (ch == 'A') ? 1U : 0U;
            if (m_FrameLen == 1U)
            {
                m_FrameBuf[0] = 'A';
            }
        }
        else if (m_FrameLen == 4U && m_FrameBuf[3] != '6')
        {
            m_FrameLen = (ch == 'A') ? 1U : 0U;
            if (m_FrameLen == 1U)
            {
                m_FrameBuf[0] = 'A';
            }
        }
        else if (ProtoFrame_HasTail())
        {
            cimc_app_process_ascii_frame(m_FrameBuf, m_FrameLen);
            m_FrameLen = 0U;
        }
        else if (m_FrameLen >= CIMC_FRAME_MAX_ASCII_LEN)
        {
            m_FrameLen = 0U;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Debug UART callback                                               */
/* ------------------------------------------------------------------ */

/**
 * @brief Callback invoked when a debug UART DMA reception completes.
 * @param data  Pointer to the received data buffer.
 * @param len   Number of bytes received.
 */
void DebugUart_RxCallback(const uint8_t *data, uint16_t len)
{
    (void)data;
    Uart_Printf(DEBUG_USART, "debug rx len=%u\r\n", len);
}

/* ------------------------------------------------------------------ */
/*  Formatted UART transmit                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief Transmit a formatted string over a UART peripheral.
 *
 * Formats the arguments with vsnprintf() and transmits the resulting
 * string byte-by-byte.  For RS-485 peripherals the RS-485 direction
 * control signal is asserted during transmission.
 *
 * @param usart_periph  GD32 USART peripheral base address.
 * @param format        printf-style format string.
 * @param ...           Variable arguments for the format string.
 * @return              Number of characters transmitted.
 */
int Uart_Printf(uint32_t usart_periph, const char *format, ...)
{
    char buffer[512];
    va_list arg;
    int len;

    va_start(arg, format);
    len = vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);

    if (usart_periph == RS232_RS485_USART || usart_periph == DEBUG_USART || usart_periph == OTA_UART_PERIPH)
    {
        RS485_CS_SET(1);
    }

    for (g_TxCount = 0; g_TxCount < len; g_TxCount++)
    {
        usart_data_transmit(usart_periph, buffer[g_TxCount]);
        while (RESET == usart_flag_get(usart_periph, USART_FLAG_TBE))
        {
        }
    }
    while (RESET == usart_flag_get(usart_periph, USART_FLAG_TC))
    {
    }

    if (usart_periph == RS232_RS485_USART || usart_periph == DEBUG_USART || usart_periph == OTA_UART_PERIPH)
    {
        RS485_CS_SET(0);
    }

    return len;
}

/* ------------------------------------------------------------------ */
/*  OTA UART DMA poll                                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Reset the OTA UART reception state.
 *
 * Clears the DMA position tracker, the circular receive buffer, the
 * DMA transfer buffer, and delegates to the OTA protocol layer.
 */
void OtaRx_Reset(void)
{
    m_OtaDmaLastPos = 0U;
    memset(rxbuffer, 0, OTA_UART_RXBUF_SIZE);
    memset(g_UartDmaBuf, 0, OTA_UART_RXBUF_SIZE);
    g_UartDmaLen = 0U;
    g_RxFlag = 0U;
    ota_uart_reset_state();
}

/**
 * @brief Poll the OTA UART DMA circular buffer for new data.
 *
 * In circular DMA mode the current write position is
 * buffer_size - NDTR.  When the DMA wraps, the tail segment is
 * processed first, then the new head segment.
 */
static void OtaUart_DmaPoll(void)
{
    uint32_t pos;

    /*
     * In circular DMA mode, the current write position is
     * buffer_size - NDTR.  When it wraps, feed the tail segment
     * first and then the new head segment.
     */
    pos = OTA_UART_RXBUF_SIZE - dma_transfer_number_get(OTA_UART_DMA, OTA_UART_DMA_CH);
    if (pos == m_OtaDmaLastPos)
    {
        return;
    }

    if (pos > m_OtaDmaLastPos)
    {
        ProtoFrame_FeedBytes(&rxbuffer[m_OtaDmaLastPos], pos - m_OtaDmaLastPos);
        ota_uart_process_frame(&rxbuffer[m_OtaDmaLastPos], pos - m_OtaDmaLastPos);
    }
    else
    {
        ProtoFrame_FeedBytes(&rxbuffer[m_OtaDmaLastPos], OTA_UART_RXBUF_SIZE - m_OtaDmaLastPos);
        ota_uart_process_frame(&rxbuffer[m_OtaDmaLastPos], OTA_UART_RXBUF_SIZE - m_OtaDmaLastPos);
        if (pos > 0U)
        {
            ProtoFrame_FeedBytes(rxbuffer, pos);
            ota_uart_process_frame(rxbuffer, pos);
        }
    }

    m_OtaDmaLastPos = pos;
}

/* ------------------------------------------------------------------ */
/*  Main UART task                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief UART periodic task, invoked every 5 ms from the scheduler.
 *
 * Handles pending debug UART reception, polls the OTA DMA circular
 * buffer for new data, and runs the OTA protocol state machine.
 */
void Uart_Process(void)
{
    if (g_DebugRxFlag)
    {
        DebugUart_RxCallback(g_DebugDmaBuf, g_DebugDmaLen);
        memset(g_DebugDmaBuf, 0, sizeof(g_DebugDmaBuf));
        g_DebugDmaLen = 0;
        g_DebugRxFlag = 0;
    }

    /* Poll raw OTA bytes first, then let the OTA parser consume
       complete frames. */
    OtaUart_DmaPoll();
    ota_uart_task();
}
