/**
 * 2026 CIMC Bootloader — 头文件
 */

#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "gd32f4xx.h"
#include <stdint.h>

/* ========== LED 引脚 ========== */
#define LED_SYS_PORT     GPIOA
#define LED_SYS_PIN      GPIO_PIN_4

/* ========== OLED 引脚 ========== */
#define OLED_SCL_PORT    GPIOB
#define OLED_SCL_PIN     GPIO_PIN_8
#define OLED_SDA_PORT    GPIOB
#define OLED_SDA_PIN     GPIO_PIN_9

/* ========== 函数声明 ========== */
void JumpToApp(uint32_t app_addr);

/* ========== SysTick 基础函数 ========== */
void systick_config(void);
void delay_1ms(uint32_t count);

/* ========== LED 基础函数 ========== */
void LED_Init(void);

/* ========== OLED 基础函数 ========== */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_ShowLine1(uint8_t *str);
void OLED_ShowLine2(uint8_t *str);

/* ========== 中断处理 ========== */
void SysTick_Handler(void);

#endif
