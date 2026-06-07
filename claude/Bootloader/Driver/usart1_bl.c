/**
 * Bootloader — USART1 驱动 (RS-485, PA2/PA3, PB12 方向控制)
 *
 * 中断接收 + 环形缓冲区, 支持波特率动态配置
 */
#include "usart1_bl.h"

/* 环形接收缓冲区 */
#define RX_BUF_SIZE  4096
static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

void USART1_BL_Init(uint32_t baud)
{
    rcu_periph_clock_enable(RCU_GPIOA);     /* USART1 TX/RX + RS-485 PA1 */
    rcu_periph_clock_enable(RCU_USART1);

    /* PA1 = 485_CS, 推挽输出, 默认接收 */
    gpio_mode_set(RS485_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, RS485_PIN);
    gpio_output_options_set(RS485_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, RS485_PIN);
    RS485_RX();

    /* PA2=TX(AF7), PA3=RX(AF7) */
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_2 | GPIO_PIN_3);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);

    usart_deinit(USART1);
    usart_baudrate_set(USART1, baud);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART1, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART1, USART_CTS_DISABLE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);

    /* 使能接收中断 */
    usart_interrupt_enable(USART1, USART_INT_RBNE);
    nvic_irq_enable(USART1_IRQn, 0, 1);

    usart_enable(USART1);

    /* 清空缓冲区 */
    rx_head = 0;
    rx_tail = 0;
}

/* 发送单字节 (阻塞, 内部使用; 对外用 SendBytes/SendString 带方向控制) */
void USART1_SendByte(uint8_t ch)
{
    while (RESET == usart_flag_get(USART1, USART_FLAG_TBE));
    usart_data_transmit(USART1, ch);
}

/* 批量发送字节 — 带方向控制 */
void USART1_SendBytes(uint8_t *buf, uint16_t len)
{
    RS485_TX();
    for (uint16_t i = 0; i < len; i++) {
        while (RESET == usart_flag_get(USART1, USART_FLAG_TBE));
        usart_data_transmit(USART1, buf[i]);
    }
    while (RESET == usart_flag_get(USART1, USART_FLAG_TC));
    RS485_RX();
}

/* 发送字符串 — 带方向控制 */
void USART1_SendString(char *str)
{
    RS485_TX();
    while (*str) {
        while (RESET == usart_flag_get(USART1, USART_FLAG_TBE));
        usart_data_transmit(USART1, (uint8_t)*str++);
    }
    while (RESET == usart_flag_get(USART1, USART_FLAG_TC));
    RS485_RX();
}

/* 读取一个字节 (非阻塞, 缓冲区空时返回 0) */
uint8_t USART1_ReadByte(void)
{
    if (rx_head == rx_tail) return 0;
    uint8_t ch = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    return ch;
}

/* 缓冲区可用字节数 */
uint16_t USART1_Available(void)
{
    return (rx_head - rx_tail + RX_BUF_SIZE) % RX_BUF_SIZE;
}

/* 清空接收缓冲区 */
void USART1_Flush(void)
{
    rx_head = 0;
    rx_tail = 0;
}

/* ================================================================
 * USART1 接收中断 — 将每个接收字节写入环形缓冲区
 * ================================================================ */
void USART1_IRQHandler(void)
{
    if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE) != RESET) {
        uint8_t ch = (uint8_t)usart_data_receive(USART1);
        uint16_t next = (rx_head + 1) % RX_BUF_SIZE;
        if (next != rx_tail) {
            rx_buf[rx_head] = ch;
            rx_head = next;
        }
        /* 缓冲区满则丢弃 (不应发生, 4096 字节足够) */
    }
}
