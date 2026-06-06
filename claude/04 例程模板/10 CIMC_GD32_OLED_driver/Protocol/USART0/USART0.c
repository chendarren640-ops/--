/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：usart0.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/3/10     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "USART0.h"

/************************* 宏定义 *************************/

/************************ 变量定义 ************************/

uint8_t data_recv = 0;  // 数据
/************************ 函数定义 ************************/

int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}

/************************************************************ 
 * Function :       USART0_Config
 * Comment  :       用于初始化usart0 相关配置
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-03-10 V0.1 original
************************************************************/
void USART0_Config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);    // 使能GPIO时钟
    rcu_periph_clock_enable(RCU_USART0);   // 使能串口时钟
	
	gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9 | GPIO_PIN_10);				//配置端口复用
	
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);  		//端口类型配置为复用
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);  

	gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_10);  	//端口类型配置为复用	
	gpio_output_options_set(GPIOA, GPIO_MODE_ANALOG, GPIO_OSPEED_50MHZ, GPIO_PIN_10);  


    usart_deinit(USART0);    						// 串口复位
    usart_word_length_set(USART0, USART_WL_8BIT);   // 字长为8位
    usart_stop_bit_set(USART0, USART_STB_1BIT);     // 停止位1位
    usart_parity_config(USART0, USART_PM_NONE);		// 无校验
    usart_baudrate_set(USART0, 115200U);     		// 波特率115200
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);     // 接收使能
	usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);   // 发送使能
	usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    usart_enable(USART0);          							// 串口使能
}


/************************************************************ 
 * Function :       USART0_SendData
 * Comment  :       用usart0发送数据
 * Parameter:       数据指针，数据长度
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-03-14 V0.2 original
************************************************************/
void USART0_SendData(uint16_t *buf,uint16_t len)
 {
     uint16_t t;
     for(t=0;t<len;t++)      
     {           
         while(usart_flag_get(USART0, USART_FLAG_TC) == RESET);  
         usart_data_transmit(USART0,buf[t]);
     }     
     while(usart_flag_get(USART0, USART_FLAG_TC) == RESET);          
}

/************************************************************ 
 * Function :       USART0_IRQHandler
 * Comment  :       串口中断服务函数，用来接收串口数据
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-03-14 V0.2 original
************************************************************/
void USART0_IRQHandler(void)
{
    if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE))
	{			
		data_recv = usart_data_receive(USART0);    //  接收串口数据
		usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE); 	//清除接收中断标志位
        process_data(data_recv); 		//处理数据
				
    }    
}

/************************************************************ 
 * Function :       process_data
 * Comment  :       其他数据则直接串口打印出来
 * Parameter:       char型的数据，只支持单字符
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-03-14 V0.2 original
************************************************************/
void process_data(uint8_t data)
{
	
		usart_data_transmit(USART0, data_recv);    //  发送数据 
	

}