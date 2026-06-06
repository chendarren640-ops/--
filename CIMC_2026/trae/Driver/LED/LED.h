/**
 * @file    LED.h
 * @brief   LED 驱动头文件
 *
 * GPIO引脚分配:
 *   LED_SYS:  系统状态灯 (1s闪烁)
 *   LED_SAMPLE: 采集工作灯 (自动上报时常亮)
 *   LED1~4:   通用LED (PA4~PA7)
 */

#ifndef __LED_H
#define __LED_H

#include "HeaderFiles.h"

/* LED 端口定义 */
#define LED1_PORT       GPIOA
#define LED1_PIN        GPIO_PIN_4
#define LED2_PORT       GPIOA
#define LED2_PIN        GPIO_PIN_5
#define LED3_PORT       GPIOA
#define LED3_PIN        GPIO_PIN_6
#define LED4_PORT       GPIOA
#define LED4_PIN        GPIO_PIN_7

/* 功能LED映射: LED2用作系统状态灯, LED3用作采集灯 */
#define LED_SYS_PORT    LED2_PORT
#define LED_SYS_PIN     LED2_PIN
#define LED_SAMPLE_PORT LED3_PORT
#define LED_SAMPLE_PIN  LED3_PIN

/* LED操作宏 */
#define LED1_ON()   gpio_bit_reset(LED1_PORT, LED1_PIN)
#define LED1_OFF()  gpio_bit_set(LED1_PORT, LED1_PIN)
#define LED1_TOG()  gpio_bit_toggle(LED1_PORT, LED1_PIN)

#define LED_SYS_ON()    gpio_bit_reset(LED_SYS_PORT, LED_SYS_PIN)
#define LED_SYS_OFF()   gpio_bit_set(LED_SYS_PORT, LED_SYS_PIN)
#define LED_SYS_TOG()   gpio_bit_toggle(LED_SYS_PORT, LED_SYS_PIN)

#define LED_SAMPLE_ON()  gpio_bit_reset(LED_SAMPLE_PORT, LED_SAMPLE_PIN)
#define LED_SAMPLE_OFF() gpio_bit_set(LED_SAMPLE_PORT, LED_SAMPLE_PIN)

void LED_Init(void);

#endif
