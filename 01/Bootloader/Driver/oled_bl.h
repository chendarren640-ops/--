/**
 * Bootloader — OLED 驱动 (精简版, 128x32 SSD1306)
 * 仅保留双行文本显示, 去除完整字符/图形功能以节省空间
 */
#ifndef __OLED_BL_H
#define __OLED_BL_H

#include "../bootloader.h"

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_ShowLine1(uint8_t *str);
void OLED_ShowLine2(uint8_t *str);

#endif
