#ifndef __MAIN_H
#define __MAIN_H
#include "gd32f4xx.h"
#include "gd32f4xx_libopt.h"
#include "systick.h"
#include "../Driver/LED/led_drv.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define STATE_IDLE        0x00
#define STATE_AUTO_SAMPLE 0x01

extern volatile uint8_t  g_sys_state;
extern volatile uint32_t g_sys_tick;
extern volatile uint8_t  g_led_toggle_flag;
extern uint8_t           g_rx_byte;

void System_Init(void);
#endif
