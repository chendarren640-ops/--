/*
 * bsp_usart_rs485.c
 * USART1 RS485 驱动 (PD5=TX, PD6=RX, PE8=DE)
 * 注意: 中断处理函数在 gd32f4xx_it.c 中实现
 */

#include "bsp_usart_rs485.h"
#include <string.h>

void BSP_RS485_Init(uint32_t baudrate)
{
    rcu_periph_clock_enable(BSP_RS485_TX_GPIO_RCU);
    rcu_periph_clock_enable(BSP_RS485_RX_GPIO_RCU);
    rcu_periph_clock_enable(BSP_RS485_DE_GPIO_RCU);
    rcu_periph_clock_enable(BSP_RS485_USART_RCU);

    /* TX 引脚配置 */
    gpio_af_set(BSP_RS485_TX_GPIO_PORT, BSP_RS485_TX_AF, BSP_RS485_TX_PIN);
    gpio_mode_set(BSP_RS485_TX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BSP_RS485_TX_PIN);
    gpio_output_options_set(BSP_RS485_TX_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_RS485_TX_PIN);

    /* RX 引脚配置 */
    gpio_af_set(BSP_RS485_RX_GPIO_PORT, BSP_RS485_RX_AF, BSP_RS485_RX_PIN);
    gpio_mode_set(BSP_RS485_RX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, BSP_RS485_RX_PIN);

    /* DE 引脚配置 (方向控制) */
    gpio_mode_set(BSP_RS485_DE_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BSP_RS485_DE_PIN);
    gpio_output_options_set(BSP_RS485_DE_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_RS485_DE_PIN);

    BSP_RS485_DE_RX();  /* 默认接收模式 */

    /* USART 配置 */
    usart_deinit(BSP_RS485_USART);
    usart_baudrate_set(BSP_RS485_USART, baudrate);
    usart_word_length_set(BSP_RS485_USART, USART_WL_8BIT);
    usart_stop_bit_set(BSP_RS485_USART, USART_STB_1BIT);
    usart_parity_config(BSP_RS485_USART, USART_PM_NONE);
    usart_hardware_flow_rts_config(BSP_RS485_USART, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(BSP_RS485_USART, USART_CTS_DISABLE);
    usart_receive_config(BSP_RS485_USART, USART_RECEIVE_ENABLE);
    usart_transmit_config(BSP_RS485_USART, USART_TRANSMIT_ENABLE);

    /* 使能接收中断 */
    usart_interrupt_enable(BSP_RS485_USART, USART_INT_RBNE);
    usart_interrupt_enable(BSP_RS485_USART, USART_INT_IDLE);

    nvic_irq_enable(BSP_RS485_IRQ, 1, 1);

    usart_enable(BSP_RS485_USART);
}

void BSP_RS485_SetBaudrate(uint32_t baudrate)
{
    usart_disable(BSP_RS485_USART);
    usart_baudrate_set(BSP_RS485_USART, baudrate);
    usart_enable(BSP_RS485_USART);
}

void BSP_RS485_SendByte(uint8_t data)
{
    uint32_t timeout;

    BSP_RS485_DE_TX();

    timeout = BSP_RS485_TX_TIMEOUT;
    while (RESET == usart_flag_get(BSP_RS485_USART, USART_FLAG_TBE))
    {
        if (timeout-- == 0) break;
    }

    usart_data_transmit(BSP_RS485_USART, data);

    timeout = BSP_RS485_TX_TIMEOUT;
    while (RESET == usart_flag_get(BSP_RS485_USART, USART_FLAG_TC))
    {
        if (timeout-- == 0) break;
    }

    BSP_RS485_DE_RX();
}

void BSP_RS485_SendBuffer(const uint8_t *buf, uint16_t len)
{
    uint16_t i;
    uint32_t timeout;

    if (buf == NULL || len == 0) return;

    BSP_RS485_DE_TX();

    for (i = 0; i < len; i++)
    {
        timeout = BSP_RS485_TX_TIMEOUT;
        while (RESET == usart_flag_get(BSP_RS485_USART, USART_FLAG_TBE))
        {
            if (timeout-- == 0) break;
        }
        usart_data_transmit(BSP_RS485_USART, buf[i]);
    }

    timeout = BSP_RS485_TX_TIMEOUT;
    while (RESET == usart_flag_get(BSP_RS485_USART, USART_FLAG_TC))
    {
        if (timeout-- == 0) break;
    }

    BSP_RS485_DE_RX();
}

void BSP_RS485_SendString(const char *str)
{
    if (str == NULL) return;
    BSP_RS485_SendBuffer((const uint8_t *)str, (uint16_t)strlen(str));
}
