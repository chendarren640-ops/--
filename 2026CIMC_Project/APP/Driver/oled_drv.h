#ifndef __OLED_DRV_H
#define __OLED_DRV_H
#include "main.h"
#define OLED_W 128
#define OLED_H 32
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_ShowLine1(uint8_t *str);
void OLED_ShowLine2(uint8_t *str);
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *str, uint8_t font_size);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t font_size);
#endif
