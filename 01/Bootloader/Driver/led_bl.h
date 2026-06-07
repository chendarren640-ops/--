/**
 * Bootloader — LED 驱动 (精简版)
 * PD10: 系统状态灯 (低电平有效)
 */
#ifndef __LED_BL_H
#define __LED_BL_H

#include "../bootloader.h"

void LED_Init(void);
#define LED_ON()  gpio_bit_reset(GPIOD, GPIO_PIN_10)
#define LED_OFF() gpio_bit_set(GPIOD, GPIO_PIN_10)

#endif
