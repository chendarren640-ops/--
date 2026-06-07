/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Function.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/31     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"
#include "usart.h"
#include "usart0.h"
#include "LED.h"	

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

	LED_Init();

	usart_init();

}

/************************************************************
 * Function :       UsrFunction
 * Comment  :       用户程序功能: LED1闪烁
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/


void UsrFunction(void)
{


	while (1)
	{

		usart_recv_buf();

		usart_send_str("485_send_OK\r\n", 14);

		delay_1ms(1000);
	}
}


/****************************End*****************************/

