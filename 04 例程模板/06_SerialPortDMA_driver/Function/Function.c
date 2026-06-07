/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Function.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/30     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"
#include "dma.h"
#include "usart.h"
#include "LED.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/


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
	LED_Init();			// LED初始化
	my_dma_init();		// DMA初始化
	usart_init();		// USART初始化
}

/************************************************************
 * Function :       UsrFunction
 * Comment  :       用户程序功能: 通过串口的DMA接收到OK(ok)就进行LED1闪烁, 接收到OFF(off)关闭LED1
 * 								并且打印接收到的数据	
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-30 V0.1 original
************************************************************/

extern uint8_t LED_Stat;
extern uint8_t recv_flag;
void UsrFunction(void)
{

	while (1)
	{
		usart_recv_buf();

		if (LED_Stat)
		{
			LED1_ON();
		}
		else
		{
			LED1_OFF();
		}

		delay_1ms(1000);
	}
}


/****************************End*****************************/

