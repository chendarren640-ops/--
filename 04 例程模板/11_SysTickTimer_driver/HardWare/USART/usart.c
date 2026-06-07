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
static uint8_t usart_send_len = 0;
static uint8_t usart_send_index = 0;

/************************ 函数定义 ************************/

/************************************************************
 * Function :       usart_init
 * Comment  :       用于初始化MCU的USART0
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-30 V0.1 original
************************************************************/
void usart_init(void)
{
    //! 首先开启对应的中断处理函数     
    nvic_irq_enable(USART0_IRQn, 3, 2);

    //! 使能USART时钟
    rcu_periph_clock_enable(USART_RCU);
    rcu_periph_clock_enable(USART_PIN_RCU);

    // 对TX配置成为复用模式
    gpio_af_set(USART_PORT, GPIO_AF_7, USART_TX_Pin);

    // 设置GPIO引脚模式
    gpio_mode_set(USART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, USART_TX_Pin);
    // 设置GPIO引脚参数
    gpio_output_options_set(USART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART_TX_Pin);

    // 配置USART
    usart_deinit(USART);
    usart_baudrate_set(USART, 115200U);
    usart_transmit_config(USART, USART_TRANSMIT_ENABLE);

    usart_enable(USART);
}

/************************************************************
 * Function :       usart_send_str
 * Comment  :       用于发送字符串到USART0
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-30 V0.1 original
************************************************************/
void usart_send_str(uint8_t* str, uint8_t len)
{
    if (len > sizeof(Buf_Send))
    {
        return;
    }

    memcpy(Buf_Send, str, len);
    // printf("Buf_Send: %s", Buf_Send);
    usart_send_len = len;
    //开启发送中断(当发送缓冲区为空)
    usart_interrupt_enable(USART, USART_INT_TBE);
}

/************************************************************
 * Function :       fputc
 * Comment  :       用于重定向printf函数到USART0
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-30 V0.1 original
************************************************************/
int fputc(int ch, FILE* f)
{
    Buf_Send[usart_send_len++] = ch;
    if (ch == '\n')
    {
        usart_send_str(Buf_Send, usart_send_len);
    }

    return ch;
}

/************************************************************
 * Function :       USART0_IRQHandler
 * Comment  :       用于发送字符串到USART0的中断处理函数
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-30 V0.1 original
************************************************************/
void USART0_IRQHandler(void)
{
    // printf("enter Interrupt\r\n");
    if (usart_interrupt_flag_get(USART, USART_INT_FLAG_TBE))
    {

        if (usart_send_index < usart_send_len)
        {
            usart_data_transmit(USART, Buf_Send[usart_send_index]);
            usart_send_index++;
        }
        else
        {
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
    }

}
