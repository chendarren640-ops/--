#ifndef __HEADERFILES_H__
#define __HEADERFILES_H__

#include "gd32f4xx.h"
#include "systick.h"
#include "gd32f4xx_libopt.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* 官方/已移植驱动 */
#include "LED.h"
#include "OLED.h"
/* 新增驱动 */
#include "bsp_adc.h"
#include "bsp_dac.h"
#include "bsp_i2c.h"
#include "gd30ad3340.h"
#include "pt100.h"
#include "bsp_flash.h"
#include "bsp_usart_rs485.h"
#include "bsp_rtc.h"

/* 协议 */
#include "protocol_ascii_hex.h"
#include "protocol_cmd.h"
#include "protocol_crc.h"

/* APP 功能 */
#include "Function.h"
#include "app_param.h"
#include "app_sample.h"
#include "app_alarm.h"
#include "app_autoreport.h"
#include "app_sleep.h"



#endif



/****************************End*****************************/

