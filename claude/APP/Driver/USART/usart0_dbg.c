#include "usart0_dbg.h"
void USART0_DBG_Init(void) {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART0);
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_10);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_10);
    usart_deinit(USART0);
    usart_baudrate_set(USART0, 19200U);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);

    /* 使能接收中断 — 上位机命令帧接收 */
    nvic_irq_enable(USART0_IRQn, 2, 2);
    usart_interrupt_enable(USART0, USART_INT_RBNE);
}

void USART0_DBG_ReconfigBaud(uint32_t baud) {
    usart_baudrate_set(USART0, baud);   /* 直接写波特率, 不破坏 USART 状态 */
}
void USART0_DBG_SendByte(uint8_t ch) {
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    usart_data_transmit(USART0, ch);
}
void USART0_DBG_SendString(char *str) {
    while(*str) USART0_DBG_SendByte((uint8_t)*str++);
}
int fputc(int ch, FILE *f) {
    USART0_DBG_SendByte((uint8_t)ch);
    return ch;
}
