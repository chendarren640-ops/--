#include "LED.h"

void LED_Init(void)
{
    /* 使能 GPIOB 时钟 */
    rcu_periph_clock_enable(RCU_GPIOB);

    /* 初始化系统状态灯：PB0 */
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_0);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
    gpio_bit_reset(GPIOB, GPIO_PIN_0);   /* 初始熄灭 */

    /* 初始化采集工作灯：PB1 */
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_1);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    gpio_bit_reset(GPIOB, GPIO_PIN_1);   /* 初始熄灭 */
}
