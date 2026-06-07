#ifndef __BSP_USART_H
#define __BSP_USART_H

#include "gd32f4xx.h"           /* GD32F4 外设库 */
#include <stdint.h>             /* 标准整型 */
#include <stdio.h>              /* 标准 I/O，用于 printf 重定向 */

/*
    默认串口：
    USART0_TX -> PA9
    USART0_RX -> PA10

    如果你的开发板不是 PA9/PA10，需要改下面的引脚宏。
*/

#define BSP_USART_RCU              RCU_USART0       /* USART0 时钟 */
#define BSP_USART                  USART0           /* USART0 外设基地址 */

#define BSP_USART_GPIO_RCU         RCU_GPIOA        /* GPIOA 时钟 */
#define BSP_USART_GPIO_PORT        GPIOA            /* GPIOA 端口 */
#define BSP_USART_TX_PIN           GPIO_PIN_9       /* TX 引脚 PA9 */
#define BSP_USART_RX_PIN           GPIO_PIN_10      /* RX 引脚 PA10 */
#define BSP_USART_AF               GPIO_AF_7        /* PA9/PA10 复用功能选择 AF7（USART0） */

/**
 * @brief  初始化 USART0，配置波特率、数据位、停止位、校验等
 * @param  baudrate  通信波特率，例如 115200
 */
void BSP_USART0_Init(uint32_t baudrate);

/**
 * @brief  通过 USART0 发送一个字节（阻塞方式）
 * @param  data  要发送的 8 位数据
 */
void BSP_USART0_SendByte(uint8_t data);

/**
 * @brief  通过 USART0 发送一个以空字符结尾的字符串
 * @param  str  要发送的字符串指针
 */
void BSP_USART0_SendString(const char *str);

#endif /* __BSP_USART_H */

