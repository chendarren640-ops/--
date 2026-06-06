/**
 * @file    HeaderFiles.h
 * @brief   CIMC 2026 全局头文件汇总
 * @note    所有业务模块通过此文件引入全部依赖
 */

#ifndef __HEADERFILES_H
#define __HEADERFILES_H

#include "gd32f4xx.h"
#include "gd32f4xx_libopt.h"
#include "systick.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Driver layer */
#include "LED.h"
#include "usart_driver.h"
#include "OLED.h"

/* Protocol layer */
#include "protocol_utils.h"

/* Function layer */
#include "function.h"

#endif
