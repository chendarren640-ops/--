/**
 * 2026 CIMC APP — LED 驱动
 */

#include "led_drv.h"

/**
 * @brief  初始化 LED 引脚 (PA5=采集灯, PA6=系统灯, PA7=备用; PA4留给DAC)
 */
void LED_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);

    /* PA5~PA7 推挽输出, PA4 留给 DAC_OUT0 模拟输出 */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);

    /* 初始全部熄灭 */
    gpio_bit_reset(GPIOA, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
}
