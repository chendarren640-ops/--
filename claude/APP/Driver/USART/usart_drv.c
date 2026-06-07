/**
 * 2026 CIMC APP — USART1 驱动 (RS-485)
 *
 * 赛题要求:
 *   - 通信接口: USART1 (PA2=TX, PA3=RX, AF7)
 *   - 默认波特率 19200, 8 数据位, 1 停止位, 无校验
 *   - 所有帧以 ASCII 十六进制字符串收发
 *   - 485_CS 方向控制: PB12 (高=发送, 低=接收)
 */

#include "usart_drv.h"

uint8_t  g_rx_byte = 0;
uint8_t  rx_buf[RX_BUF_SIZE];
volatile uint16_t rx_head = 0;
volatile uint16_t rx_tail = 0;

void USART1_Config(void) {
    /* === 时钟 === */
    rcu_periph_clock_enable(RCU_GPIOA);     /* USART1 TX/RX */
    rcu_periph_clock_enable(RCU_GPIOB);     /* RS-485 方向控制 PB12 */
    rcu_periph_clock_enable(RCU_USART1);

    /* RS-485 方向控制: PB12, 推挽输出, 默认接收 */
    gpio_mode_set(RS485_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, RS485_PIN);
    gpio_output_options_set(RS485_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, RS485_PIN);
    RS485_RX();

    /* PA2(TX) — AF7 推挽输出 */
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_2);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);

    /* PA3(RX) — AF7 输入上拉 (RS-485 空闲时 RO 可能浮空) */
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_3);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_3);

    /* USART1 配置: 19200-8-N-1 */
    usart_deinit(USART1);
    usart_baudrate_set(USART1, USART1_BAUDRATE);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART1, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART1, USART_CTS_DISABLE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_enable(USART1);

    /* 使能接收中断 (RBNE = 接收缓冲区非空) */
    nvic_irq_enable(USART1_IRQn, 0, 0);
    usart_interrupt_enable(USART1, USART_INT_RBNE);
}

void USART1_Config_Baud(uint32_t baud) {
    usart_disable(USART1);
    usart_baudrate_set(USART1, baud);
    usart_enable(USART1);
}

void USART1_SendByte(uint8_t ch) {
    while (RESET == usart_flag_get(USART1, USART_FLAG_TBE));
    usart_data_transmit(USART1, ch);
}

void USART1_SendBytes(uint8_t *buf, uint16_t len) {
    RS485_TX();
    for (uint16_t i = 0; i < len; i++) USART1_SendByte(buf[i]);
    while (RESET == usart_flag_get(USART1, USART_FLAG_TC));
    RS485_RX();
}

void USART1_SendString(char *str) {
    RS485_TX();
    while (*str) USART1_SendByte((uint8_t)*str++);
    while (RESET == usart_flag_get(USART1, USART_FLAG_TC));
    RS485_RX();
}

uint16_t USART1_RecvBytes(uint8_t *buf, uint16_t max_len) {
    uint16_t cnt = 0;
    while (rx_head != rx_tail && cnt < max_len) {
        buf[cnt++] = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    }
    return cnt;
}

