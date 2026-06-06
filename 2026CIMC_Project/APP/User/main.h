#ifndef __MAIN_H
#define __MAIN_H
#include "gd32f4xx.h"
#include "gd32f4xx_libopt.h"
#include "systick.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* 系统状态 */
#define STATE_IDLE        0x00
#define STATE_AUTO_SAMPLE 0x01

/* 全局变量 */
extern volatile uint8_t  g_sys_state;
extern volatile uint32_t g_sys_tick;
extern volatile uint8_t  g_led_toggle_flag;
extern uint8_t           g_rx_byte;

/* 函数 */
void System_Init(void);
#endif
