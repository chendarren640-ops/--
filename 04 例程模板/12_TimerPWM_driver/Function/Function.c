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

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/
// 占空比翻转标志位
uint8_t duty_flip_flag = 0;

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
}

/************************************************************ 
 * Function :       UsrFunction
 * Comment  :       用户程序功能: 通过PWM来实现呼吸灯效果
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.01 original
************************************************************/
void UsrFunction(void)
{
	
	uint8_t duty = 0;	// 占空比
	// 占空比范围: 0-99%
	//通过不断修改占空比,实现呼吸灯效果
	while(1)
	{
		if(duty_flip_flag == 0)
		{
			duty++;
		}
		else
		{
			duty--;
		}
		if(duty >= 99)
		{
			duty_flip_flag = 1;
		}
		else if(duty == 0)
		{
			duty_flip_flag = 0;
		}
		my_timer_set_duty(duty);
		delay_1ms(15);
	}
}


/****************************End*****************************/

