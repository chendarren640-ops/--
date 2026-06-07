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

extern uint32_t ic1value , ic2value;
extern __IO uint16_t dutycycle;
uint8_t i = 0;
/************************ 函数定义 ************************/



/************************************************************
 * Function :       System_Init
 * Comment  :       用于初始化MCU
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/

void System_Init(void)
{
	systick_config();     // 时钟配置

	my_timer_init();	//定时器的初始化

	my_usart_init();
	delay_1ms(100);
}

/************************************************************
 * Function :       UsrFunction
 * Comment  :       用户程序功能: 计算占空比
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/

void UsrFunction(void)
{

	printf("test begin。。。。。\r\n");
	while (1)
	{

		printf("占空比 = %d%%\r\n" , dutycycle);
		dutycycle = 0;
		delay_1ms(1000);
	}
}


/****************************End*****************************/

