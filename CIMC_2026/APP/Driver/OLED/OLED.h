/**
 * @file    OLED.h
 * @brief   0.96寸 OLED SSD1306 驱动 (GPIO模拟I2C)
 *
 * 赛题要求: 双行显示
 *   第一行: 队伍编号
 *   第二行: 状态信息 (Bootloader/IDLE/AutoSample)
 */

#ifndef __OLED_H
#define __OLED_H

#include "gd32f4xx.h"
#include <stdint.h>

/* GPIO 引脚定义 - PB8(SCL), PB9(SDA) */
#define OLED_SCL_PORT    GPIOB
#define OLED_SCL_PIN     GPIO_PIN_8
#define OLED_SDA_PORT    GPIOB
#define OLED_SDA_PIN     GPIO_PIN_9

/* I2C控制宏 */
#define OLED_SCLK_Set()  gpio_bit_set(OLED_SCL_PORT, OLED_SCL_PIN)
#define OLED_SCLK_Clr()  gpio_bit_reset(OLED_SCL_PORT, OLED_SCL_PIN)
#define OLED_SDIN_Set()  gpio_bit_set(OLED_SDA_PORT, OLED_SDA_PIN)
#define OLED_SDIN_Clr()  gpio_bit_reset(OLED_SDA_PORT, OLED_SDA_PIN)
#define IIC_READ_SDA     gpio_input_bit_get(OLED_SDA_PORT, OLED_SDA_PIN)

#define OLED_CMD  0
#define OLED_DATA 1

/* 类型别名 */
#define u8  uint8_t
#define u32 uint32_t

/* 像素缓冲 128x32 */
extern u8 OLED_GRAM[144][4];

/* 基础操作 */
void OLED_WR_Byte(u8 dat, u8 mode);
void OLED_Refresh(void);
void OLED_Clear(void);

/* 显示函数 */
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size1);
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 size1);
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size1);
void OLED_ShowChinese(u8 x, u8 y, u8 num, u8 size1);

/* 绘图函数 */
void OLED_DrawPoint(u8 x, u8 y);
void OLED_ClearPoint(u8 x, u8 y);
void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2);
void OLED_DrawCircle(u8 x, u8 y, u8 r);

/* 初始化和控制 */
void OLED_Init(void);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_ColorTurn(u8 i);
void OLED_DisplayTurn(u8 i);

#endif
