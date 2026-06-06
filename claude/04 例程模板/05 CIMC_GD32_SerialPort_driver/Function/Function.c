/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：Function.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"

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
	
	USART0_Config();     // 串口初始化
	
	
	nvic_irq_enable(USART0_IRQn, 0, 0);//使能USART0中断
	
	usart_interrupt_enable(USART0, USART_INT_RBNE);//接收中断打开
	
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
	
	printf("CIMC Sys Init\r\n");  //串口打印文字
	
	while(1)
	{
		LED1_OFF();
		
		delay_1ms(200);
		
		LED1_ON();
		
		delay_1ms(200);
		
	
	}
}


/****************************End*****************************/

