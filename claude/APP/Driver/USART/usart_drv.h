#ifndef __USART_DRV_H
#define __USART_DRV_H
#include "main.h"

#define USART1_BAUDRATE  19200U
#define RX_BUF_SIZE       512

/* RS-485 方向控制 (MAX3485 DE/RE → PA1, 高=发送, 低=接收) */
#define RS485_PORT   GPIOA
#define RS485_PIN    GPIO_PIN_1
#define RS485_TX()   gpio_bit_set(RS485_PORT, RS485_PIN)
#define RS485_RX()   gpio_bit_reset(RS485_PORT, RS485_PIN)

extern uint8_t g_rx_byte;
extern uint8_t rx_buf[RX_BUF_SIZE];
extern volatile uint16_t rx_head;
extern volatile uint16_t rx_tail;

void     USART1_Config(void);
void     USART1_Config_Baud(uint32_t baud);
void     USART1_SendByte(uint8_t ch);
void     USART1_SendBytes(uint8_t *buf, uint16_t len);
void     USART1_SendString(char *str);
uint16_t USART1_RecvBytes(uint8_t *buf, uint16_t max_len);
void     USART1_Flush(void);
int      fputc(int ch, FILE *f);
#endif
