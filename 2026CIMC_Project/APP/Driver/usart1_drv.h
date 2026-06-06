#ifndef __USART1_DRV_H
#define __USART1_DRV_H
#include "main.h"

#define USART1_BAUDRATE  19200U
#define RX_BUF_SIZE       512

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
int      fputc(int ch, FILE *f);
#endif
