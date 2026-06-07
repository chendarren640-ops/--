/**
 * 2026 CIMC APP — LED 驱动
 */

#include "led_drv.h"

/**
 * @brief  初始化 LED 引脚 (PD8=采集灯, PD10=系统灯, PD11=备用; PA4留给DAC)
 */
void LED_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOD);

    /* PD8~PD13 推挽输出, 初始全部熄灭 (高电平 = 灭, 低电平有效) */
    gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                  GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                            GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);

    /* 初始全部熄灭 (低电平有效: 高电平 = 灭) */
    gpio_bit_set(GPIOD, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                         GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);
}
