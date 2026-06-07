/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：Function.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/29     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"
#include "LED.h"
#include "usart.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/
//每一秒加一
uint32_t second_count = 0;
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

	LED_Init();          // LED初始化

	usart_init();          // 串口初始化
}

/************************************************************ 
 * Function :       UsrFunction
 * Comment  :       用户程序功能: 
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.01 original
************************************************************/
void UsrFunction(void)
{
	
	while(1)
	{
		delay_1ms(1000);
		printf("second_count = %d\r\n", second_count);
	}
}


/****************************End*****************************/

