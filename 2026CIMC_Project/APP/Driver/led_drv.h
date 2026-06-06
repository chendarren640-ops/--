#ifndef __LED_DRV_H
#define __LED_DRV_H
#include "main.h"
#define LED_SYS_PORT     GPIOA
#define LED_SYS_PIN      GPIO_PIN_4
#define LED_SAMPLE_PORT  GPIOA
#define LED_SAMPLE_PIN   GPIO_PIN_5
#define LED_SYS_ON()     gpio_bit_set(LED_SYS_PORT, LED_SYS_PIN)
#define LED_SYS_OFF()    gpio_bit_reset(LED_SYS_PORT, LED_SYS_PIN)
#define LED_SAMPLE_ON()  gpio_bit_set(LED_SAMPLE_PORT, LED_SAMPLE_PIN)
#define LED_SAMPLE_OFF() gpio_bit_reset(LED_SAMPLE_PORT, LED_SAMPLE_PIN)
void LED_Init(void);
#endif
