/**
 * 2026 CIMC APP — 按键驱动
 */

#include "key_drv.h"

/**
 * @brief  初始化按键引脚 (PE15/PE13/PE11/PE9/PE7, PB0, PA0)
 *         全部配置为上拉输入 (低电平有效)
 */
void KEY_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOA);

    /* PE15/PE13/PE11/PE9/PE7 — 上拉输入 */
    gpio_mode_set(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                  GPIO_PIN_15 | GPIO_PIN_13 | GPIO_PIN_11 |
                  GPIO_PIN_9  | GPIO_PIN_7);

    /* PB0 — 上拉输入 */
    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_0);

    /* PA0 (WKUP) — 上拉输入 */
    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_0);
}

/**
 * @brief  按键扫描 (带简易消抖)
 * @return 按下的按键号 1~6, 0=无按键
 */
uint8_t KEY_Scan(void)
{
    static uint32_t last_tick = 0;

    /* 20ms 消抖间隔 */
    if (g_sys_tick - last_tick < 20) return 0;
    last_tick = g_sys_tick;

    if (KEY1_READ() == 0) return 1;
    if (KEY2_READ() == 0) return 2;
    if (KEY3_READ() == 0) return 3;
    if (KEY4_READ() == 0) return 4;
    if (KEY5_READ() == 0) return 5;
    if (KEY6_READ() == 0) return 6;

    return 0;
}
