/**
 * Bootloader — USART1 驱动 (RS-485, PA2/PA3, PB12 方向控制)
 */
#ifndef __USART1_BL_H
#define __USART1_BL_H
#include "../bootloader.h"

/* RS-485 方向控制 (485_CS → PB12) */
#define RS485_PORT   GPIOB
#define RS485_PIN    GPIO_PIN_12
#define RS485_TX()   gpio_bit_set(RS485_PORT, RS485_PIN)
#define RS485_RX()   gpio_bit_reset(RS485_PORT, RS485_PIN)


void     USART1_BL_Init(uint32_t baud);
void     USART1_SendByte(uint8_t ch);
void     USART1_SendBytes(uint8_t *buf, uint16_t len);
void     USART1_SendString(char *str);
uint8_t  USART1_ReadByte(void);
uint16_t USART1_Available(void);
void     USART1_Flush(void);

#endif
