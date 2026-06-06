/**
 * @file    usart_driver.c
 * @brief   USART1 串口驱动实现 (RS485, 19200-8N1)
 *
 * 关键规则:
 *   1. 默认波特率必须 19200, 否则上位机无法连接
 *   2. 所有协议帧以 ASCII 字符串形式收发 (非 HEX 发送)
 *   3. printf 重定向到 USART1
 */

#include "usart_driver.h"

/* 全局接收缓冲区 */
volatile uint8_t  usart_rx_buf[USART_RX_BUF_SIZE];
volatile uint16_t usart_rx_len = 0;
volatile uint8_t  usart_rx_complete = 0;

/**
 * @brief printf 重定向到 USART1
 */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART1, (uint8_t)ch);
    while(RESET == usart_flag_get(USART1, USART_FLAG_TBE));
    return ch;
}

/**
 * @brief USART1 初始化 19200-8N1
 * @note  PA2=TX(AF7), PA3=RX(AF7)
 *        RS485 使用 MAX3485, 方向控制引脚需根据实际硬件配置
 */
void USART1_Config(void)
{
    /* 使能 GPIOA 和 USART1 时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART1);

    /* PA2(TX), PA3(RX) 复用为 AF7 */
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_2 | GPIO_PIN_3);

    /* PA2: 复用推挽输出 */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);

    /* PA3: 复用输入 */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);

    /* 串口配置: 19200-8N1, 无流控 */
    usart_deinit(USART1);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_baudrate_set(USART1, USART_BAUDRATE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_hardware_flow_rts_config(USART1, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART1, USART_CTS_DISABLE);
    usart_enable(USART1);

    /* 使能 USART1 接收中断 */
    nvic_irq_enable(USART1_IRQn, 1, 0);
    usart_interrupt_enable(USART1, USART_INT_RBNE);
}

/**
 * @brief 发送 ASCII 字符串 (不封装帧, 用于 printf 输出)
 */
void USART1_SendString(char *str)
{
    while(*str) {
        usart_data_transmit(USART1, (uint8_t)*str);
        while(RESET == usart_flag_get(USART1, USART_FLAG_TBE));
        str++;
    }
}

/**
 * @brief 发送十六进制帧 (原始HEX转为ASCII字符串后发送)
 * @note  赛题要求: 帧数据必须转成ASCII字符串发送, 如 0xA5->'A''5'
 */
void USART1_SendHexFrame(uint8_t *data, uint16_t len)
{
    (void)data;
    (void)len;
    /* 此函数由 Protocol 层的 frame_send() 替代 */
    /* 这里预留, 方便直接调试用 */
}

/**
 * @brief USART1 中断处理
 */
void USART1_IRQHandler(void)
{
    if(RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE)) {
        uint8_t data = usart_data_receive(USART1);
        usart_interrupt_flag_clear(USART1, USART_INT_FLAG_RBNE);

        /* 简单回显: 收到什么回什么 (初期调试用) */
        usart_data_transmit(USART1, data);
        while(RESET == usart_flag_get(USART1, USART_FLAG_TBE));

        /* 存储到接收缓冲 */
        if(usart_rx_len < USART_RX_BUF_SIZE - 1) {
            usart_rx_buf[usart_rx_len++] = data;
        }
    }
}
