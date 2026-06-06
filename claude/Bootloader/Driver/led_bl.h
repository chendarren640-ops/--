/**
 * Bootloader — LED 驱动 (精简版)
 * PA4: 系统状态灯
 */
#ifndef __LED_BL_H
#define __LED_BL_H

#include "bootloader.h"

void LED_Init(void);
#define LED_ON()  gpio_bit_set(GPIOA, GPIO_PIN_4)
#define LED_OFF() gpio_bit_reset(GPIOA, GPIO_PIN_4)

#endif
