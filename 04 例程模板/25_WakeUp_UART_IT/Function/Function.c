/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Function.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2026/1/30     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"
#include "LED.h"
#include "usart.h"



/************************* 宏定义 *************************/


/************************ 变量定义 ************************/

/************************ 函数定义 ************************/



/************************************************************
 * Function :       System_Init
 * Comment  :       用于初始化MCU
 * Parameter:       null
 * Return   :       null
 * Author   :      	Jialei Zhao
 * Date     :       2026-01-30 V0.1 original
************************************************************/

void System_Init(void)
{
	// systick_config();     // 时钟配置

	LED_Init();        // LED初始化

	usart_init();
}
/************************************************************
 * Function :       UsrFunction
 * Comment  :       用户程序功能: LED1闪烁
 * Parameter:       null
 * Return   :       null
 * Author   :      	Jialei Zhao
 * Date     :       2026-01-30 V0.1 original
************************************************************/
void UsrFunction(void)
{
	rcu_periph_clock_enable(RCU_PMU);

	printf("process enter sleep mode ....\r\n");
	//!会阻塞 等待任意中断唤醒
	pmu_to_sleepmode(WFI_CMD);
	printf("process wake up ....\r\n");

	//!睡眠模式(非深度睡眠模式) 唤醒后 时钟不会改变

	while (1)
	{

		LED1_ON();

	}
}

/****************************End*****************************/
