/**
 * Bootloader — LED 初始化
 */
#include "led_bl.h"

void LED_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOD);
    gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_10);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
    gpio_bit_set(GPIOD, GPIO_PIN_10);   /* 初始熄灭 (低电平有效: 高=灭) */
}
