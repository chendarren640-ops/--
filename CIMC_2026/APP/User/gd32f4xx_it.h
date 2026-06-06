/**
 * @file    gd32f4xx_it.h
 * @brief   中断服务函数头文件
 */

#ifndef __GD32F4XX_IT_H
#define __GD32F4XX_IT_H

#include "gd32f4xx.h"

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

#endif
