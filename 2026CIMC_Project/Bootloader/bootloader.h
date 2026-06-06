#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H
#include "gd32f4xx.h"
#include <stdint.h>
#define APP_ADDR 0x08011000
#define MAGIC_WORD 0x5AA5C33C
void JumpToApp(uint32_t addr);
/* 驱动 */
void systick_config(void);
void delay_1ms(uint32_t count);
void LED_Init(void);
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_ShowLine1(uint8_t *str);
void OLED_ShowLine2(uint8_t *str);
void USART1_BL_Init(void);
#endif
