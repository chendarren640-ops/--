#ifndef __SYS_INIT_H
#define __SYS_INIT_H
#include "main.h"
#include "systick.h"
#include "../Driver/LED/led_drv.h"
#include "../Driver/OLED/oled_drv.h"
#include "../Driver/USART/usart_drv.h"
#include "../Driver/ADC/adc_drv.h"
#include "../Driver/DAC/dac_drv.h"
#include "../Driver/RTC/rtc_drv.h"
#include "../Driver/Flash/flash_param.h"
#include "../Driver/KEY/key_drv.h"
#include "cmd_handler.h"
void System_Init(void);
#endif
