/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：Function.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"
#include "LED.h"
#include "RTC.h"
#include "USART.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/


/************************ 函数定义 ************************/



/************************************************************ 
 * Function :       System_Init
 * Comment  :       用于初始化MCU
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/

void System_Init(void)
{
	systick_config();     // 时钟配置
	
	gd_eval_com_init();   // 串口初始化
	
  LED_Init();			  // LED初始化
		
	RTC_Init();		      // RTC初始化
	
}

/************************************************************ 
 * Function :       UsrFunction
 * Comment  :       用户程序功能
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/

void UsrFunction(void)
{
		
	
	while(1)
	{
			rtc_show_time();
			delay_1ms(1000);
	}
}


/****************************End*****************************/

