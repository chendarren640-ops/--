/* SPDX-License-Identifier: MIT */

/**
 * @file    gd32f4xx_it.c
 * @brief   Interrupt service routines for GD32F4xx.
 */

#include "isr_defs.h"
#include "main_defs.h"
#include "board_defs.h"
#include "tick_timer.h"
#include "string.h"
#include "usart_app.h"

extern uint8_t g_RxBuf[OTA_UART_RXBUF_SIZE];
extern uint8_t g_DebugRxBuf[DBG_RXBUF_SIZE];
extern uint8_t uart_dma_buffer[OTA_UART_RXBUF_SIZE];
extern uint8_t g_DebugDmaBuf[DBG_RXBUF_SIZE];
extern __IO uint8_t rx_flag;
extern __IO uint16_t uart_dma_len;
extern __IO uint8_t g_DebugRxFlag;
extern __IO uint16_t g_DebugDmaLen;

/**
 * @brief   This function handles NMI exception.
 */
void NMI_Handler(void)
{
    /* If NMI exception occurs, go to infinite loop */
    while (1)
    {
    }
}

/**
 * @brief   This function handles HardFault exception.
 */
void HardFault_Handler(void)
{
    /* If Hard Fault exception occurs, go to infinite loop */
    while (1)
    {
    }
}

/**
 * @brief   This function handles MemManage exception.
 */
void MemManage_Handler(void)
{
    /* If Memory Manage exception occurs, go to infinite loop */
    while (1)
    {
    }
}

/**
 * @brief   This function handles BusFault exception.
 */
void BusFault_Handler(void)
{
    /* If Bus Fault exception occurs, go to infinite loop */
    while (1)
    {
    }
}

/**
 * @brief   This function handles UsageFault exception.
 */
void UsageFault_Handler(void)
{
    /* If Usage Fault exception occurs, go to infinite loop */
    while (1)
    {
    }
}

/**
 * @brief   This function handles SVC exception.
 */
void SVC_Handler(void)
{
    /* If SVC exception occurs, go to infinite loop */
    while (1)
    {
    }
}

/**
 * @brief   This function handles DebugMon exception.
 */
void DebugMon_Handler(void)
{
    /* If DebugMon exception occurs, go to infinite loop */
    while (1)
    {
    }
}

/**
 * @brief   This function handles PendSV exception.
 */
void PendSV_Handler(void)
{
    /* If PendSV exception occurs, go to infinite loop */
    while (1)
    {
    }
}

static void ota_uart_irq_process(void)
{
    if (RESET != usart_interrupt_flag_get(OTA_UART_PERIPH, USART_INT_FLAG_IDLE))
    {
        /* Clear IDLE flag */
        usart_data_receive(OTA_UART_PERIPH);
    }
}

#if (PROTO_UART_IRQN != 37)
void USART0_IRQHandler(void)
{
    uint32_t rx_len;

    if (RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE))
    {
        /* Clear IDLE flag */
        usart_data_receive(USART0);

        rx_len = DBG_RXBUF_SIZE - dma_transfer_number_get(DBG_RX_DMA, DBG_RX_DMA_CH);
        if ((rx_len > 0U) && (rx_len <= DBG_RXBUF_SIZE) && (g_DebugRxFlag == 0U))
        {
            memcpy(g_DebugDmaBuf, g_DebugRxBuf, rx_len);
            g_DebugDmaLen = (uint16_t)rx_len;
            g_DebugRxFlag = 1U;
        }

        memset(g_DebugRxBuf, 0, DBG_RXBUF_SIZE);
        dma_channel_disable(DBG_RX_DMA, DBG_RX_DMA_CH);
        dma_flag_clear(DBG_RX_DMA, DBG_RX_DMA_CH, DMA_FLAG_FTF);
        dma_transfer_number_config(DBG_RX_DMA, DBG_RX_DMA_CH, DBG_RXBUF_SIZE);
        dma_channel_enable(DBG_RX_DMA, DBG_RX_DMA_CH);
    }
}
#endif
#if (PROTO_UART_IRQN == 38)
void USART1_IRQHandler(void)
{
    ota_uart_irq_process();
}
#elif (PROTO_UART_IRQN == 39)
void USART2_IRQHandler(void)
{
    ota_uart_irq_process();
}
#endif

void EXTI0_IRQHandler(void)
{
    if (RESET != exti_interrupt_flag_get(EXTI_0))
    {
        exti_interrupt_flag_clear(EXTI_0);
    }
}

void RTC_WKUP_IRQHandler(void)
{
    if (RESET != exti_interrupt_flag_get(EXTI_22))
    {
        exti_interrupt_flag_clear(EXTI_22);
    }

    if (SET == rtc_flag_get(RTC_FLAG_WT))
    {
        rtc_flag_clear(RTC_FLAG_WT);
    }
}

/**
 * @brief   This function handles SysTick exception.
 */
void SysTick_Handler(void)
{
    Tick_Decrement();
}
