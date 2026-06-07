/*
 * bsp_usart_rs485.h
 * USART1 RS485 驱动头文件
 * TX: PD5 (AF7), RX: PD6 (AF7), DE: PE8
 */

#ifndef __BSP_USART_RS485_H
#define __BSP_USART_RS485_H

#include "gd32f4xx.h"
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================= 硬件引脚定义 ================= */
#define BSP_RS485_USART                  USART1
#define BSP_RS485_USART_RCU              RCU_USART1

/* TX: PD5 (AF7) */
#define BSP_RS485_TX_GPIO_RCU            RCU_GPIOD
#define BSP_RS485_TX_GPIO_PORT           GPIOD
#define BSP_RS485_TX_PIN                 GPIO_PIN_5
#define BSP_RS485_TX_AF                  GPIO_AF_7

/* RX: PD6 (AF7) */
#define BSP_RS485_RX_GPIO_RCU            RCU_GPIOD
#define BSP_RS485_RX_GPIO_PORT           GPIOD
#define BSP_RS485_RX_PIN                 GPIO_PIN_6
#define BSP_RS485_RX_AF                  GPIO_AF_7

/* DE/RE 方向控制: PE8 */
#define BSP_RS485_DE_GPIO_RCU            RCU_GPIOE
#define BSP_RS485_DE_GPIO_PORT           GPIOE
#define BSP_RS485_DE_PIN                 GPIO_PIN_8

/* 中断 */
#define BSP_RS485_IRQ                    USART1_IRQn
#define BSP_RS485_IRQHandler             USART1_IRQHandler

/* 发送超时 */
#define BSP_RS485_TX_TIMEOUT             0x00FFFFFFUL

/* 方向控制宏 */
#define BSP_RS485_DE_RX()                gpio_bit_reset(BSP_RS485_DE_GPIO_PORT, BSP_RS485_DE_PIN)
#define BSP_RS485_DE_TX()                gpio_bit_set(BSP_RS485_DE_GPIO_PORT, BSP_RS485_DE_PIN)

/* 函数声明 */
void BSP_RS485_Init(uint32_t baudrate);
void BSP_RS485_SetBaudrate(uint32_t baudrate);
void BSP_RS485_SendByte(uint8_t data);
void BSP_RS485_SendBuffer(const uint8_t *buf, uint16_t len);
void BSP_RS485_SendString(const char *str);
void BSP_RS485_IRQHandler(void);

/* 数据发送别名 */
#define BSP_RS485_SendData  BSP_RS485_SendBuffer

#ifdef __cplusplus
}
#endif

#endif
