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

int adc_value;    // ADC采样数字量
float Vol_Value;  // ADC采样值转换成电压值

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
	
	USART0_Config();     // 串口初始化
	
	ADC_port_init(); //ADC初始化
	
	nvic_irq_enable(USART0_IRQn, 0, 0);//使能USART0中断
	
	usart_interrupt_enable(USART0, USART_INT_RBNE);//接收中断打开
	

}



/************************************************************ 
 * Function :       UsrFunction
 * Comment  :       用户程序功能: 打印ADC数值
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/

void UsrFunction(void)
{
	
	printf("CIMC Sys Init\r\n");
	while(1)
	{
		adc_flag_clear(ADC0,ADC_FLAG_EOC);  				//  清除结束标志
		while(SET != adc_flag_get(ADC0,ADC_FLAG_EOC)){}  	//  获取转换结束标志
        
        adc_value = ADC_RDATA(ADC0);    					// 读取ADC数据
		Vol_Value = adc_value*3.3/4095;  					// 把数字量转化为工程量
				
		printf("数字量为=%d	工程量为%.2f V\r\n",adc_value, Vol_Value);   // 结果打印
				
        delay_1ms(1000);  //  等待10ms
		
	}
}


/****************************End*****************************/

