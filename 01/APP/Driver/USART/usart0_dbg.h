#ifndef __USART0_DBG_H
#define __USART0_DBG_H
#include "main.h"
void USART0_DBG_Init(void);
void USART0_DBG_ReconfigBaud(uint32_t baud);
void USART0_DBG_SendString(char *str);
void USART0_DBG_SendByte(uint8_t ch);
int  fputc(int ch, FILE *f);
#endif
