/* SPDX-License-Identifier: MIT */

/**
 * @file    oled.h
 * @brief   SSD1306 0.91" OLED display driver — I2C DMA interface.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define DISPLAY_I2C_ADDR  0x78

#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT  32

void Display_WriteCmd(uint8_t cmd);
void Display_WriteData(uint8_t data);
void Display_ShowPic(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t BMP[]);
void Display_ShowHanzi(uint8_t x, uint8_t y, uint8_t no);
void Display_ShowHzbig(uint8_t x, uint8_t y, uint8_t n);
void Display_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t accuracy, uint8_t fontsize);
void Display_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t length, uint8_t fontsize);
void Display_ShowStr(uint8_t x, uint8_t y, char *ch, uint8_t fontsize);
void Display_ShowChar(uint8_t x, uint8_t y, uint8_t ch, uint8_t fontsize);
void Display_Fill(void);
void Display_SetPos(uint8_t x, uint8_t y);
void Display_Clear(void);
void Display_On(void);
void Display_Off(void);
void Display_Init(void);

#ifdef __cplusplus
}
#endif
