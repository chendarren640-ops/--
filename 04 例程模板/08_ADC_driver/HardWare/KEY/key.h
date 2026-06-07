/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：key.h
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/
#ifndef __KEY_H
#define __KEY_H

/************************* 头文件 *************************/

#include "HeaderFiles.h"

/************************* 宏定义 *************************/


#define   KEY_Press()     (RESET == gpio_input_bit_get(GPIOE,GPIO_PIN_2))     // 按键     按下
#define   KEY_Release()   (SET == gpio_input_bit_get(GPIOE,GPIO_PIN_2))       // 按键     弹起
 
/************************ 变量定义 ************************/


/************************ 函数定义 ************************/

void KEY_Init(void);     // LED 初始化
void EXTI_PIN_Init(void);    // ext interrupt init

				    
#endif


/****************************End*****************************/

