/******************************************************************************
 * 文件: bsp_usart_rs485.c
 * 说明: RS485 驱动，基于 USART2 (PD5 TX, PD6 RX)
 *       配合 bsp_usart_rs485.h 使用 (已修改为 USART2)
 *****************************************************************************/
#include "bsp_usart_rs485.h"

static volatile uint8_t  s_rs485_rx_buf[BSP_RS485_RX_BUF_SIZE];
static volatile uint16_t s_rs485_rx_len = 0;
static volatile uint8_t  s_rs485_frame_done = 0;

void BSP_RS485_Init(uint32_t baudrate)
{
    /* 使能相关时钟 */
    rcu_periph_clock_enable(BSP_RS485_TX_GPIO_RCU);
    rcu_periph_clock_enable(BSP_RS485_RX_GPIO_RCU);
    rcu_periph_clock_enable(BSP_RS485_DE_GPIO_RCU);
    rcu_periph_clock_enable(BSP_RS485_USART_RCU);

    /* 配置 TX 引脚 (PD5)：复用推挽输出 */
    gpio_af_set(BSP_RS485_TX_GPIO_PORT, BSP_RS485_TX_AF, BSP_RS485_TX_PIN);
    gpio_mode_set(BSP_RS485_TX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, BSP_RS485_TX_PIN);
    gpio_output_options_set(BSP_RS485_TX_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_RS485_TX_PIN);

    /* 配置 RX 引脚 (PD6)：复用浮空输入（不需要 output_options） */
    gpio_af_set(BSP_RS485_RX_GPIO_PORT, BSP_RS485_RX_AF, BSP_RS485_RX_PIN);
    gpio_mode_set(BSP_RS485_RX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, BSP_RS485_RX_PIN);
    /* 注意：接收引脚不要调用 gpio_output_options_set */

    /* 配置 DE 引脚 (如 PA1)：普通推挽输出 */
    gpio_mode_set(BSP_RS485_DE_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BSP_RS485_DE_PIN);
    gpio_output_options_set(BSP_RS485_DE_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_RS485_DE_PIN);

    /* 默认接收模式 */
    BSP_RS485_DE_RX();

    /* USART 初始化 */
    usart_deinit(BSP_RS485_USART);
    usart_baudrate_set(BSP_RS485_USART, baudrate);
    usart_word_length_set(BSP_RS485_USART, USART_WL_8BIT);
    usart_stop_bit_set(BSP_RS485_USART, USART_STB_1BIT);
    usart_parity_config(BSP_RS485_USART, USART_PM_NONE);
    usart_hardware_flow_rts_config(BSP_RS485_USART, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(BSP_RS485_USART, USART_CTS_DISABLE);
    usart_receive_config(BSP_RS485_USART, USART_RECEIVE_ENABLE);
    usart_transmit_config(BSP_RS485_USART, USART_TRANSMIT_ENABLE);

    /* 使能中断：接收非空和空闲 */
    usart_interrupt_enable(BSP_RS485_USART, USART_INT_RBNE);
    usart_interrupt_enable(BSP_RS485_USART, USART_INT_IDLE);

    /* NVIC 中断配置 */
    nvic_irq_enable(BSP_RS485_IRQ, 1, 1);

    /* 使能 USART */
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

    if (buf == 0 || len == 0) return;

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
    if (str == 0) return;
    BSP_RS485_SendBuffer((const uint8_t *)str, (uint16_t)strlen(str));
}

uint8_t BSP_RS485_GetFrame(uint8_t *buf, uint16_t *len)
{
    uint16_t i;

    if (buf == 0 || len == 0) return 0;

    if (s_rs485_frame_done == 0) return 0;

    __disable_irq();

    *len = s_rs485_rx_len;
    for (i = 0; i < s_rs485_rx_len; i++)
    {
        buf[i] = s_rs485_rx_buf[i];
    }

    s_rs485_rx_len = 0;
    s_rs485_frame_done = 0;

    __enable_irq();

    return 1;
}

void BSP_RS485_ClearFrame(void)
{
    __disable_irq();
    s_rs485_rx_len = 0;
    s_rs485_frame_done = 0;
    __enable_irq();
}

/* 中断服务函数：宏 BSP_RS485_IRQHandler 在 .h 中定义为 USART2_IRQHandler */
void BSP_RS485_IRQHandler(void)
{
    uint8_t data;

    if (RESET != usart_interrupt_flag_get(BSP_RS485_USART, USART_INT_FLAG_RBNE))
    {
        data = (uint8_t)usart_data_receive(BSP_RS485_USART);

        if (s_rs485_frame_done == 0)
        {
            if (s_rs485_rx_len < BSP_RS485_RX_BUF_SIZE)
            {
                s_rs485_rx_buf[s_rs485_rx_len++] = data;
            }
            else
            {
                s_rs485_rx_len = 0;
            }
        }
    }

    if (RESET != usart_interrupt_flag_get(BSP_RS485_USART, USART_INT_FLAG_IDLE))
    {
        volatile uint32_t tmp;
        tmp = USART_STAT0(BSP_RS485_USART);
        tmp = USART_DATA(BSP_RS485_USART);
        (void)tmp;

        if (s_rs485_rx_len > 0)
        {
            s_rs485_frame_done = 1;
        }
    }
}


