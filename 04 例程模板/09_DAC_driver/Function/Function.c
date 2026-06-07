/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Function.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/31     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"
#include "adc.h"
#include "usart.h"
/************************* 宏定义 *************************/


/************************ 变量定义 ************************/
extern uint16_t g_adc_value;

/************************ 函数定义 ************************/



/************************************************************
 * Function :       System_Init
 * Comment  :       用于初始化MCU
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-31 V0.1 original
************************************************************/

void System_Init(void)
{
	systick_config();     // 时钟配置

	LED_Init();			// 初始化LED

	ADC_Init();	// 初始化ADC(里面初始化了DAC)

	my_usart_init();	// 初始化串口

}
/************************************************************
 * Function :       Init_LED_Stat
 * Comment  :       系统初始化时用LED显示状态
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-31 V0.1 original
************************************************************/

void Init_LED_Stat(void)
{
	//功能说明：
	//当系统上电后，执行下面的程序
	//LED1~4间隔1秒依次点亮后关断
	//用来指示系统上电后进入用户程序的动作

	LED2_ON();
	delay_1ms(1000);
	LED3_ON();
	delay_1ms(1000);
	LED4_ON();
	delay_1ms(1000);
	LED2_OFF();
	LED3_OFF();
	LED4_OFF();

}
/************************************************************
 * Function :       UsrFunction
 * Comment  :       用户程序功能: LED1闪烁
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-31 V0.1 original
************************************************************/


void UsrFunction(void)
{
	Init_LED_Stat();

	printf("Hello World!\r\n");
	while (1)
	{

		//!在ADC转换完成后，将转换结果赋值给g_adc_value
		uint16_t adc_value = ADC_Read_Register();
		float dac_value = adc_value* 3.3 / 4095;

		printf("ADC: %d , 电压:%.2f V\r\n", adc_value, dac_value);


		delay_1ms(1000);
	}
}


/****************************End*****************************/

