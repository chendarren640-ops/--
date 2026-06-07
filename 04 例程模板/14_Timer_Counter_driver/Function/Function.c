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

extern __IO uint32_t count;

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
 * Comment  :       用户程序功能: 实现记录指定时间内脉冲上升沿的次数
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.01 original
************************************************************/
void UsrFunction(void)
{

	printf("如下实验记录1S内上升沿的次数\r\n");
	printf("test begin。。。。。\r\n");
	while (1)
	{

		printf("1S内上升沿的次数(count) = %d\r\n" , count);

		count = 0;


		delay_1ms(1000);
	}
}


/****************************End*****************************/

