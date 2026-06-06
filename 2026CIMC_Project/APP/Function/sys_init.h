#ifndef __SYS_INIT_H
#define __SYS_INIT_H
#include "main.h"
#include "systick.h"
#include "led_drv.h"
#include "oled_drv.h"
#include "usart1_drv.h"
#include "adc_drv.h"
#include "dac_drv.h"
#include "rtc_drv.h"
#include "flash_param.h"
#include "cmd_handler.h"
void System_Init(void);
#endif
