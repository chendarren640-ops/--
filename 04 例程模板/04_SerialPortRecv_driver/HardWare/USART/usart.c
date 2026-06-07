/************************************************************
 * 版权：2026CIMC Copyright。
 * 文件：usart.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/30     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "usart.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/
 //! 接收数据的缓冲区
uint8_t recv_buf[128] = { 0 };
uint8_t recv_len = 0;
//! 接收数据的数组
uint8_t recv_real_buf[128] = { 0 };
uint8_t recv_real_len = 0;
//! 接收数据的标记位
uint8_t recv_flag = 0;

extern uint8_t LED_Stat;	// LED状态变量,根据串口接收到的数据进行控制	默认是关闭状态
/************************ 函数定义 ************************/



/************************************************************
 * Function :       usart_init
 * Comment  :       用于初始化USART0
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-30 V0.1 original
************************************************************/
void usart_init(void)
{
	//! 首先开启对应的中断处理函数     
	nvic_irq_enable(USART0_IRQn , 3 , 2);

	//! 使能USART时钟
	rcu_periph_clock_enable(USART_RCU);
	rcu_periph_clock_enable(USART_PIN_RCU);

	// 对TX配置成为复用模式
	gpio_af_set(USART_PORT , GPIO_AF_7 , USART_RX_Pin);

	// 设置GPIO引脚模式
	gpio_mode_set(USART_PORT , GPIO_MODE_AF , GPIO_PUPD_NONE , USART_RX_Pin);
	// 设置GPIO引脚参数
	gpio_output_options_set(USART_PORT , GPIO_OTYPE_PP , GPIO_OSPEED_50MHZ , USART_RX_Pin);

	// 配置USART
	usart_deinit(USART);
	usart_baudrate_set(USART , 115200U);
	usart_receive_config(USART , USART_RECEIVE_ENABLE);
	usart_enable(USART);

	//! 启动串口中断
	usart_interrupt_enable(USART , USART_INT_RBNE);	//接收缓冲区非空中断
	usart_interrupt_enable(USART , USART_INT_IDLE);	//空闲中断

}

/************************************************************
 * Function :       usart_recv_buf
 * Comment  :       用于处理USART0接收缓冲区数据
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-30 V0.1 original
************************************************************/
void usart_recv_buf(void)
{
	//判断接收到的数据是否是OK/ok是OFF/off
	if (recv_flag)
	{
		if (memcmp(recv_real_buf , "OK" , 2) == 0 || memcmp(recv_real_buf , "ok" , 2) == 0)
		{
			LED_Stat = 1;
		}
		else if (memcmp(recv_real_buf , "OFF" , 3) == 0 || memcmp(recv_real_buf , "off" , 3) == 0)
		{
			LED_Stat = 0;
		}
		recv_flag = 0;
	}
}

/************************************************************
 * Function :       USART0_IRQHandler
 * Comment  :       用于处理USART0中断
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-30 V0.1 original
************************************************************/
void USART0_IRQHandler(void)
{
	// 接收缓冲区非空中断 ， 则就进行缓冲区的数据读取 ， 后存放在缓冲区中
	if (usart_interrupt_flag_get(USART , USART_INT_FLAG_RBNE) != RESET)
	{
		recv_buf[recv_len++] = usart_data_receive(USART);
	}
	// 进行不定长数据的接收
	if (usart_interrupt_flag_get(USART , USART_INT_FLAG_IDLE) != RESET)
	{
		//清除IDLE中断标志位
		usart_data_receive(USART);
		if (recv_len != 0)
		{
			memcpy(recv_real_buf , recv_buf , recv_len);
			recv_real_len = recv_len;
			recv_len = 0;
			recv_flag = 1;
		}
	}
}

/****************************End*****************************/
