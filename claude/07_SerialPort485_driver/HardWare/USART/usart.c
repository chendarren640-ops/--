/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：usart.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/30     V0.01    original
************************************************************/


/************************* 头文件 *************************/
#include "usart.h"

/************************* 宏定义 *************************/



/************************ 变量定义 ************************/
static uint8_t Buf_Send[128];
__IO static uint8_t usart_send_len = 0;
__IO static uint8_t usart_send_index = 0;
__IO uint8_t usart_recvSuccess_flag = 1;

uint8_t recv_buf[128] = { 0 };
uint8_t recv_len = 0;
uint8_t recv_real_buf[128] = { 0 };
uint8_t recv_real_len = 0;
uint8_t recv_flag = 0;
uint8_t usart_led_state = 0;

__IO static uint8_t send_busy = 0;   

/************************ 函数定义 ************************/
void usart_485_CS(uint8_t cs);

void usart_init(void)
{
    nvic_irq_enable(USART1_IRQn, 5, 0);
    rcu_periph_clock_enable(USARTX_RCU);
    rcu_periph_clock_enable(USART_PIN_RCU);
    rcu_periph_clock_enable(USART_485_CS_RCU);

    gpio_mode_set(USART_485_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, USARTX_485_CS_Pin);
    gpio_output_options_set(USART_485_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USARTX_485_CS_Pin);

    gpio_af_set(USART_PORT, GPIO_AF_7, USART_TX_Pin);
    gpio_mode_set(USART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, USART_TX_Pin);
    gpio_output_options_set(USART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART_TX_Pin);

    gpio_af_set(USART_PORT, GPIO_AF_7, USART_RX_Pin);
    gpio_mode_set(USART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, USART_RX_Pin);
    gpio_output_options_set(USART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART_RX_Pin);

    usart_deinit(USART);
    usart_baudrate_set(USART, 115200U);
    usart_transmit_config(USART, USART_TRANSMIT_ENABLE);
    usart_receive_config(USART, USART_RECEIVE_ENABLE);

    usart_interrupt_enable(USART, USART_INT_RBNE);
    usart_interrupt_enable(USART, USART_INT_IDLE);
    usart_enable(USART);
}

void usart_send_str(uint8_t* str, uint8_t len)
{
    if (len > sizeof(Buf_Send))
    {
        return;
    }

    while (send_busy);
    send_busy = 1;

    usart_485_CS(USARTX_485_Send);

    usart_interrupt_disable(USART, USART_INT_RBNE);
    usart_interrupt_disable(USART, USART_INT_IDLE);
    memcpy(Buf_Send, str, len);
    usart_send_len = len;
    usart_interrupt_enable(USART, USART_INT_TBE);
}

void usart_recv_buf(void)
{
    if (recv_flag)
    {
        usart_send_str("485_RECV_OK\r\n", 14);
        usart_send_str(recv_real_buf, recv_real_len);
        recv_flag = 0;
    }
}

void usart_485_CS(uint8_t cs)
{
    if (cs == 1)
    {
        gpio_bit_set(USART_485_CS_PORT, USARTX_485_CS_Pin);
    }
    else
    {
        gpio_bit_reset(USART_485_CS_PORT, USARTX_485_CS_Pin);
    }
    // delay_1ms(1);
}

void USART1_IRQHandler(void)
{
    if (usart_interrupt_flag_get(USART, USART_INT_FLAG_TBE))
    {
        if (usart_send_index < usart_send_len)
        {
            usart_data_transmit(USART, Buf_Send[usart_send_index]);
            usart_send_index++;
        }
        else
        {
            Buf_Send[usart_send_index] = '\0';
            usart_send_index = 0;
            usart_send_len = 0;
            usart_interrupt_disable(USART, USART_INT_TBE);
            usart_interrupt_enable(USART, USART_INT_TC);
        }
    }

    if (usart_interrupt_flag_get(USART, USART_INT_FLAG_TC))
    {
        usart_interrupt_flag_clear(USART, USART_INT_FLAG_TC);
        usart_interrupt_disable(USART, USART_INT_TC);
        usart_485_CS(USARTX_485_Receive);
        usart_interrupt_enable(USART, USART_INT_RBNE);
        usart_interrupt_enable(USART, USART_INT_IDLE);
        send_busy = 0;   
    }

    if (usart_interrupt_flag_get(USART, USART_INT_FLAG_RBNE) != RESET)
    {
        usart_recvSuccess_flag = 0;
        if (recv_len < sizeof(recv_buf))
        {
            recv_buf[recv_len++] = usart_data_receive(USART);
        }
    }

    if (usart_interrupt_flag_get(USART, USART_INT_FLAG_IDLE) != RESET && recv_len != 0)
    {
        memcpy(recv_real_buf, recv_buf, recv_len);
        recv_real_len = recv_len;
        recv_real_buf[recv_real_len] = '\0';
        recv_len = 0;
        recv_flag = 1;
        usart_data_receive(USART);
        usart_recvSuccess_flag = 1;
    }
}


