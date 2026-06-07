/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Function.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/29      V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"
#include "timer.h"
#include "fwdgt.h"
#include "usart.h"
#include "key.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/
extern uint8_t fdgt_flag;

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

	my_fwdgt_init();

	KEY_Init();
}

/************************************************************
 * Function :       UsrFunction
 * Comment  :       用户程序功能: 实现在规定时间内喂狗，
 *                  如果超过规定时间没有喂狗，
 *                  则触发独立看门狗复位。
 * 					通过定时器喂狗
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/

void UsrFunction(void)
{

	printf("Program Start , 已经复位\r\n");


	while (1)
	{
		if(fdgt_flag)
		{
			printf("success feed the dog\r\n");
			fdgt_flag = 0;
		}
		else
		{
			printf("no feed the dog\r\n");
		}
		delay_1ms(1000);
	}
}


/****************************End*****************************/

