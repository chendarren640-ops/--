/**
 * @file    LED.c
 * @brief   LED 驱动实现
 */

#include "LED.h"

void LED_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);

    /* PA4~PA7 初始化为推挽输出, 低电平点亮 */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    /* 初始全部熄灭 */
    gpio_bit_set(GPIOA, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
}
