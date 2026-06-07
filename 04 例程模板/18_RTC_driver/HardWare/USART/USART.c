/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：USART.c
 * 作者: Qiao Qin @ GigaDevice
 * 平台: 2025CIMC IHD-V04
 * 版本: Qiao Qin     2025/4/20     V0.01    original
************************************************************/
#include "USART.h"

/*!
    \brief      初始化串口0（USART0）的GPIO和基本参数
    \param[in]  无
    \param[out] 无
    \retval     无
*/
void gd_eval_com_init(void)
{
    /* 使能GPIOA端口的时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);

    /* 使能USART0外设的时钟 */
    rcu_periph_clock_enable(RCU_USART0);

    /* 将PA9和PA10引脚复用功能配置为USART0的TX和RX */
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9);   // PA9 -> USART0_TX
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_10);  // PA10 -> USART0_RX

    /* 配置PA9为复用推挽输出，并开启内部上拉 */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);

    /* 配置PA10为复用推挽输出（输入模式自动由USART外设控制），并开启内部上拉 */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    /* 复位USART0所有配置至默认状态 */
    usart_deinit(USART0);
    /* 设置波特率为115200 */
    usart_baudrate_set(USART0, 115200U);
    /* 使能接收功能 */
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    /* 使能发送功能 */
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    /* 使能USART0 */
    usart_enable(USART0);
}

/*!
    \brief      重定向C标准库printf函数到底层USART发送
    \param[in]  ch  : 要发送的字符
    \param[in]  f   : 文件指针（未使用）
    \retval     返回发送的字符
    \note       配合标准库使用，使printf可以直接通过串口输出
*/
int fputc(int ch, FILE *f)
{
    /* 将字符通过USART0发送出去 */
    usart_data_transmit(USART0, (uint8_t)ch);
    /* 等待发送缓冲区空（TBE标志置位），确保字符完整发出 */
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}
