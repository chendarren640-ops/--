#ifndef __BSP_USART_RS485_H
#define __BSP_USART_RS485_H

#include "gd32f4xx.h"
#include <stdint.h>
#include <string.h>

/*
 * USART2 RS485 配置：PD5 TX, PD6 RX, DE: PA1
 * 恢复原始配置
 */
#define BSP_RS485_USART                  USART2
#define BSP_RS485_USART_RCU              RCU_USART2

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

/* DE/RE 方向控制: PA1 */
#define BSP_RS485_DE_GPIO_RCU            RCU_GPIOA
#define BSP_RS485_DE_GPIO_PORT           GPIOA
#define BSP_RS485_DE_PIN                 GPIO_PIN_1

#define BSP_RS485_IRQ                    USART2_IRQn
#define BSP_RS485_IRQHandler             USART2_IRQHandler

#define BSP_RS485_RX_BUF_SIZE            1024
#define BSP_RS485_TX_TIMEOUT             0x00FFFFFFUL

#define BSP_RS485_DE_RX()                gpio_bit_reset(BSP_RS485_DE_GPIO_PORT, BSP_RS485_DE_PIN)
#define BSP_RS485_DE_TX()                gpio_bit_set(BSP_RS485_DE_GPIO_PORT, BSP_RS485_DE_PIN)

#ifdef __cplusplus
extern "C" {
#endif

void BSP_RS485_Init(uint32_t baudrate);
void BSP_RS485_SetBaudrate(uint32_t baudrate);

void BSP_RS485_SendByte(uint8_t data);
void BSP_RS485_SendBuffer(const uint8_t *buf, uint16_t len);
void BSP_RS485_SendString(const char *str);

uint8_t BSP_RS485_GetFrame(uint8_t *buf, uint16_t *len);
void BSP_RS485_ClearFrame(void);

#ifdef __cplusplus
}
#endif

#endif
