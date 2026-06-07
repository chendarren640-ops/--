/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Function.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/29     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"
#include "timer.h"
#include "usart.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/


extern __IO uint16_t fre;

/************************ 函数定义 ************************/



/************************************************************
 * Function :       System_Init
 * Comment  :       用于初始化MCU
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.01 original
************************************************************/

void System_Init(void)
{
	systick_config();     // 时钟配置

	my_timer_init();	//定时器的初始化

	my_usart_init();
}

/************************************************************
 * Function :       UsrFunction
 * Comment  :       用户程序功能: 计算脉冲的频率
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.01 original
************************************************************/

void UsrFunction(void)
{

	printf("test begin。。。。。\r\n");

	while (1)
	{

		printf("计算后频率 = %dHz,实际测量频率 = %dHZ\r\n", my_timer_get_freq(), fre);

		delay_1ms(500);
	}
}


/****************************End*****************************/

