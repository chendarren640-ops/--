#include "usart1_bl.h"
void USART1_BL_Init(void) {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART1);
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_2|GPIO_PIN_3);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
    usart_deinit(USART1);
    usart_baudrate_set(USART1, 115200U);  /* OTA固件心跳是115200 */
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART1, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART1, USART_CTS_DISABLE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_enable(USART1);
}
void USART1_SendByte(uint8_t ch) {
    while(RESET==usart_flag_get(USART1,USART_FLAG_TBE));
    usart_data_transmit(USART1,ch);
}
void USART1_SendString(char *str) {
    while(*str) USART1_SendByte((uint8_t)*str++);
}
