/**
 * 2026 CIMC APP — LED 驱动头文件
 *
 * LED 引脚: PA5~PA7 (PA4 留给 DAC_OUT0 模拟输出)
 *   - PA5: 采集工作指示灯 (常亮=采集, 灭=空闲)
 *   - PA6: 系统心跳指示灯 (1s 周期闪烁)
 *   - PA7: 备用
 */

#ifndef __LED_DRV_H
#define __LED_DRV_H

#include "main.h"

/* LED 引脚宏 */
#define LED_SYS_PORT      GPIOA
#define LED_SYS_PIN       GPIO_PIN_6
#define LED_SAMPLE_PORT   GPIOA
#define LED_SAMPLE_PIN    GPIO_PIN_5
#define LED_BACKUP1_PORT  GPIOA
#define LED_BACKUP1_PIN   GPIO_PIN_0  /* PA0, 与 LED_SYS(PA6) 独立 */
#define LED_BACKUP2_PORT  GPIOA
#define LED_BACKUP2_PIN   GPIO_PIN_7

/* LED 操作宏 */
#define LED_SYS_ON()      gpio_bit_set(LED_SYS_PORT, LED_SYS_PIN)
#define LED_SYS_OFF()     gpio_bit_reset(LED_SYS_PORT, LED_SYS_PIN)
#define LED_SYS_TOGGLE()  gpio_bit_toggle(LED_SYS_PORT, LED_SYS_PIN)

#define LED_SAMPLE_ON()   gpio_bit_set(LED_SAMPLE_PORT, LED_SAMPLE_PIN)
#define LED_SAMPLE_OFF()  gpio_bit_reset(LED_SAMPLE_PORT, LED_SAMPLE_PIN)

/* 函数声明 */
void LED_Init(void);

#endif
