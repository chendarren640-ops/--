#ifndef __LED_H
#define __LED_H

#include "HeaderFiles.h"


// 系统状态指示灯：PB0，进入APP后以1s为单位闪烁
#define LED_SYS_OFF()      gpio_bit_reset(GPIOB, GPIO_PIN_0)
#define LED_SYS_ON()       gpio_bit_set(GPIOB, GPIO_PIN_0)
#define LED_SYS_TOGGLE()   gpio_bit_write(GPIOB, GPIO_PIN_0, \
                           (gpio_input_bit_get(GPIOB, GPIO_PIN_0) == SET) ? RESET : SET)


// 采集工作指示灯：PB1，自动上报时常亮，其余时刻熄灭
#define LED_WORK_OFF()     gpio_bit_reset(GPIOB, GPIO_PIN_1)
#define LED_WORK_ON()      gpio_bit_set(GPIOB, GPIO_PIN_1)

void LED_Init(void);

#endif


