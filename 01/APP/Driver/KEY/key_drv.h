/**
 * 2026 CIMC APP — 按键驱动头文件
 *
 * 按键引脚:
 *   KEY1 → PE15, KEY2 → PE13, KEY3 → PE11
 *   KEY4 → PE9,  KEY5 → PE7,  KEY6 → PB0
 *   WKUP → PA0 (唤醒按键)
 */

#ifndef __KEY_DRV_H
#define __KEY_DRV_H
#include "main.h"

/* 按键引脚宏 */
#define KEY1_PORT   GPIOE
#define KEY1_PIN    GPIO_PIN_15
#define KEY2_PORT   GPIOE
#define KEY2_PIN    GPIO_PIN_13
#define KEY3_PORT   GPIOE
#define KEY3_PIN    GPIO_PIN_11
#define KEY4_PORT   GPIOE
#define KEY4_PIN    GPIO_PIN_9
#define KEY5_PORT   GPIOE
#define KEY5_PIN    GPIO_PIN_7
#define KEY6_PORT   GPIOB
#define KEY6_PIN    GPIO_PIN_0
#define WKUP_PORT   GPIOA
#define WKUP_PIN    GPIO_PIN_0

/* 按键读取 (低电平有效, 按下=0, 松开=1) */
#define KEY1_READ()  gpio_input_bit_get(KEY1_PORT, KEY1_PIN)
#define KEY2_READ()  gpio_input_bit_get(KEY2_PORT, KEY2_PIN)
#define KEY3_READ()  gpio_input_bit_get(KEY3_PORT, KEY3_PIN)
#define KEY4_READ()  gpio_input_bit_get(KEY4_PORT, KEY4_PIN)
#define KEY5_READ()  gpio_input_bit_get(KEY5_PORT, KEY5_PIN)
#define KEY6_READ()  gpio_input_bit_get(KEY6_PORT, KEY6_PIN)
#define WKUP_READ()  gpio_input_bit_get(WKUP_PORT, WKUP_PIN)

/* 函数声明 */
void KEY_Init(void);
uint8_t KEY_Scan(void);    /* 返回按下的按键号 (1~6), 0=无按键 */

#endif
