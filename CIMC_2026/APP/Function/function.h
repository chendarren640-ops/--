/**
 * @file    function.h
 * @brief   业务逻辑层头文件
 */

#ifndef __FUNCTION_H
#define __FUNCTION_H

#include "HeaderFiles.h"

/* 系统状态定义 */
#define SYS_STATUS_IDLE       0x00
#define SYS_STATUS_AUTO_SAMPLE 0x01
#define SYS_STATUS_BOOTLOADER 0x02

/* 全局变量 */
extern volatile uint32_t g_sys_tick_ms;
extern uint32_t g_team_id;
extern uint8_t g_sys_status;
extern uint8_t g_app_running;

/* 函数声明 */
void System_Init(void);
void UsrFunction(void);
void Init_LED_Stat(void);

#endif
