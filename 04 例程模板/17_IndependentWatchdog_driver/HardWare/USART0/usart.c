/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：usart.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/29      V0.01    original
************************************************************/

/************************* 头文件 *************************/
#include "usart.h"
#include "stdio.h"

/************************* 宏定义 *************************/

/************************ 变量定义 ************************/

/************************************************************
 * Function :       my_usart_init
 * Comment  :       用于初始化MCU的USART0 只有输出引脚
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/

void my_usart_init(void) {


    //! 使能USART时钟
    rcu_periph_clock_enable(RCU_USART0);
    rcu_periph_clock_enable(RCU_GPIOA);

    // 对TX配置成为复用模式
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9);

    // 设置GPIO引脚模式
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_9);
    // 设置GPIO引脚参数
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);

    // 配置USART
    usart_deinit(USART0);
    usart_baudrate_set(USART0, 115200U);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);

}

/************************************************************
 * Function :       fputc
 * Comment  :       用户程序功能: 实现USART0的字符输出
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
int fputc(int ch, FILE* f)
{
    usart_data_transmit(USART0, (uint8_t)ch);

    while (usart_flag_get(USART0, USART_FLAG_TBE) == RESET)
    {
        ;
    }
    return ch;
}


/****************************End*****************************/


