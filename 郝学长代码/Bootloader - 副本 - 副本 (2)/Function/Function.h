/*
 * Function.h
 * Bootloader 业务逻辑层头文件
 */

#ifndef __FUNCTION_H
#define __FUNCTION_H

#include "HeaderFiles.h"

/* 超时配置 */
#define BOOT_WAIT_UPDATE_TIME_MS        5000U
#define BOOT_OLED_REFRESH_TIME_MS       500U
#define BOOT_RX_FRAME_TIMEOUT_MS        100U

/* 状态类型 */
typedef enum
{
    BOOT_RUN_WAIT = 0,
    BOOT_RUN_UPDATE,
    BOOT_RUN_JUMP_APP
} boot_run_state_t;

/* 函数声明 */
void System_Init(void);
void UsrFunction(void);

#endif
