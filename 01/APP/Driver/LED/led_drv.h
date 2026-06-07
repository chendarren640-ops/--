/**
 * 2026 CIMC APP — LED 驱动头文件
 *
 * LED 引脚: PD8~PD13 (PA4 留给 DAC_OUT0 模拟输出)
 *   - PD8:  采集工作指示灯 (常亮=采集, 灭=空闲)
 *   - PD10: 系统心跳指示灯 (1s 周期闪烁)
 *   - PD11: 备用
 *   - PD9/PD12/PD13: 扩展 LED
 */

#ifndef __LED_DRV_H
#define __LED_DRV_H

#include "main.h"

/* LED 引脚宏 */
#define LED_SYS_PORT      GPIOD
#define LED_SYS_PIN       GPIO_PIN_10
#define LED_SAMPLE_PORT   GPIOD
#define LED_SAMPLE_PIN    GPIO_PIN_8
#define LED_BACKUP1_PORT  GPIOD
#define LED_BACKUP1_PIN   GPIO_PIN_11
#define LED_BACKUP2_PORT  GPIOD
#define LED_BACKUP2_PIN   GPIO_PIN_12

/* LED 操作宏 (低电平有效) */
#define LED_SYS_ON()      gpio_bit_reset(LED_SYS_PORT, LED_SYS_PIN)
#define LED_SYS_OFF()     gpio_bit_set(LED_SYS_PORT, LED_SYS_PIN)
#define LED_SYS_TOGGLE()  gpio_bit_toggle(LED_SYS_PORT, LED_SYS_PIN)

#define LED_SAMPLE_ON()   gpio_bit_reset(LED_SAMPLE_PORT, LED_SAMPLE_PIN)
#define LED_SAMPLE_OFF()  gpio_bit_set(LED_SAMPLE_PORT, LED_SAMPLE_PIN)

/* 函数声明 */
void LED_Init(void);

#endif
