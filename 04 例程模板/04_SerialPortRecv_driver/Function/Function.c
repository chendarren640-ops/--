/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Function.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/30     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"
#include "usart.h"
#include "LED.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/
uint8_t LED_Stat = 0;	// LED状态变量,根据串口接收到的数据进行控制	默认是关闭状态


/************************ 函数定义 ************************/



/************************************************************
 * Function :       System_Init
 * Comment  :       用于初始化MCU
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-30 V0.1 original
************************************************************/

void System_Init(void)
{
	systick_config();     // 时钟配置

	LED_Init();			// 初始化LED

	usart_init();		// 初始化串口
}

/************************************************************
 * Function :       UsrFunction
 * Comment  :       用户程序功能: 接收串口数据，根据数据控制LED状态
 * 					OK / ok:  LED6_ON
 * 					OFF / off: LED6_OFF
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-30 V0.1 original
************************************************************/
void UsrFunction(void)
{
	while (1)
	{
		//对串口接收到的数据进行判断是OK/ok是OFF/off
		usart_recv_buf();

		if (LED_Stat)
		{
			LED4_ON();	//开启LED4
		}
		else
		{
			LED4_OFF();	//关闭LED4
		}

		delay_1ms(500);
	}
}


/****************************End*****************************/

