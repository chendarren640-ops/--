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
#include "rtc.h"



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
	systick_config();     // 时钟配置

	LED_Init();        // LED初始化

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
	LED1_ON();

	delay_1ms(3000);

	LED1_OFF();

	rcu_periph_clock_enable(RCU_PMU);

	my_rtc_init();

	/* clear STBF bit */
	pmu_flag_clear(PMU_FLAG_RESET_STANDBY);
	//!会阻塞 等待任意中断唤醒
	/* PMU enters standby mode */
	pmu_to_standbymode();
	//!待机模式，唤醒后相当于复位 从头执行
	//!所以下面代码跑不到

	while (1)
	{

	}
}

/****************************End*****************************/
