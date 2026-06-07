#include "bsp_usart.h"

/**
 * @brief  初始化 USART0 外设
 * @param  baudrate  波特率，例如 115200、9600 等
 * @note   默认使用 PA9 作为 TX，PA10 作为 RX，配置为：
 *         - 8 位数据位
 *         - 1 位停止位
 *         - 无校验
 *         - 无硬件流控
 *         - 使能发送和接收
 */
void BSP_USART0_Init(uint32_t baudrate)
{
    /* 使能 GPIO 和 USART 时钟 */
    rcu_periph_clock_enable(BSP_USART_GPIO_RCU);
    rcu_periph_clock_enable(BSP_USART_RCU);

    /*
        PA9  -> USART0_TX
        PA10 -> USART0_RX
        配置引脚复用功能为 USART0
    */
    gpio_af_set(BSP_USART_GPIO_PORT,
                BSP_USART_AF,
                BSP_USART_TX_PIN | BSP_USART_RX_PIN);

    /* 配置引脚模式为复用推挽，内部上拉 */
    gpio_mode_set(BSP_USART_GPIO_PORT,
                  GPIO_MODE_AF,
                  GPIO_PUPD_PULLUP,
                  BSP_USART_TX_PIN | BSP_USART_RX_PIN);

    /* 设置输出选项：推挽输出，最大速度 50MHz */
    gpio_output_options_set(BSP_USART_GPIO_PORT,
                            GPIO_OTYPE_PP,
                            GPIO_OSPEED_50MHZ,
                            BSP_USART_TX_PIN | BSP_USART_RX_PIN);

    /* 复位 USART 外设至默认状态 */
    usart_deinit(BSP_USART);

    /* 设置波特率 */
    usart_baudrate_set(BSP_USART, baudrate);
    /* 8 位数据位 */
    usart_word_length_set(BSP_USART, USART_WL_8BIT);
    /* 1 位停止位 */
    usart_stop_bit_set(BSP_USART, USART_STB_1BIT);
    /* 无校验 */
    usart_parity_config(BSP_USART, USART_PM_NONE);

    /* 禁用 RTS 和 CTS 硬件流控制 */
    usart_hardware_flow_rts_config(BSP_USART, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(BSP_USART, USART_CTS_DISABLE);

    /* 使能接收和发送 */
    usart_receive_config(BSP_USART, USART_RECEIVE_ENABLE);
    usart_transmit_config(BSP_USART, USART_TRANSMIT_ENABLE);

    /* 最终使能 USART */
    usart_enable(BSP_USART);
}

/**
 * @brief  通过 USART0 发送一个字节
 * @param  data  要发送的 8 位数据
 * @note   该函数为阻塞式，会等待发送缓冲区为空后再写入数据，
 *         并等待数据发送完成才返回。
 */
void BSP_USART0_SendByte(uint8_t data)
{
    /* 写入数据到发送数据寄存器 */
    usart_data_transmit(BSP_USART, data);

    /* 等待发送缓冲区空标志（TBE = Transmit Buffer Empty） */
    while(RESET == usart_flag_get(BSP_USART, USART_FLAG_TBE))
    {
    }
}

/**
 * @brief  通过 USART0 发送一个字符串
 * @param  str  以空字符 '\0' 结尾的字符串指针
 * @note   内部逐个字节调用 BSP_USART0_SendByte 发送，
 *         直到遇到字符串结束符。
 */
void BSP_USART0_SendString(const char *str)
{
    while(*str)
    {
        BSP_USART0_SendByte((uint8_t)(*str));
        str++;                 /* 移动到下一个字符 */
    }
}

/*
    printf 重定向到 USART0

    通过重新实现 fputc，使得标准库的 printf 函数通过串口输出。
    Keil AC5 / AC6 一般都可以用这个。
*/
int fputc(int ch, FILE *f)
{
    /* 发送字符 ch */
    BSP_USART0_SendByte((uint8_t)ch);
    return ch;
}

