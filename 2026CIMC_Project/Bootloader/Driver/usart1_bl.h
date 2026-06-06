#ifndef __USART1_BL_H
#define __USART1_BL_H
#include "bootloader.h"
void USART1_BL_Init(void);
void USART1_SendByte(uint8_t ch);
void USART1_SendString(char *str);
#endif
