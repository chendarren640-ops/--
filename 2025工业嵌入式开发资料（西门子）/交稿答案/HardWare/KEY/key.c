/************************* 头文件 *************************/

#include "KEY.h"

/************************ 全局变量定义 ************************/


/************************************************************ 
 * Function :       KEY_Init
 * Comment  :       用于初始化key端口
************************************************************/

void KEY_Init(void)
{
	
    rcu_periph_clock_enable(KEY_CLK);								// 初始化KEY总线时钟
    
    gpio_mode_set(KEY_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                 KEY1_PIN | KEY2_PIN | KEY3_PIN | KEY4_PIN);		//配置GPIO模式为输入
	
	
}


/************************************************************ 
 * Function :       KEY_Stat
 * Comment  :       用于读取按键状态
 * Parameter:       按键端口和引脚
 * Return   :       按键状态：1为按键按下，0为按键未按下
************************************************************/
uint8_t KEY_Stat(uint32_t port, uint16_t pin)
{
    if(gpio_input_bit_get(port, pin) == RESET)					//读取GPIO状态，如果按键被按下了
    {
        delay_1ms(20);											//延时消抖
        if(gpio_input_bit_get(port, pin) == RESET)				//再读一次GPIO，按键真的被按下了吗？
        {
            //while(gpio_input_bit_get(port, pin) == RESET);		//等待按键释放
            return 1;											//返回按键状态1
        }
    }
    return 0;
}

/************************************************************ 
 * Function :       EXTI_PIN_Init
 * Comment  :       用于初始化中断
************************************************************/
void EXTI_PIN_Init(void)
{
	rcu_periph_clock_enable(RCU_SYSCFG);								//使能SYSCFG时钟
	
	syscfg_exti_line_config(EXTI_SOURCE_GPIOE, EXTI_SOURCE_PIN2);		//连接PE2端口到中断线
	
	exti_init(EXTI_2, EXTI_INTERRUPT, EXTI_TRIG_FALLING);				//配置中断为下降沿触发
	
	nvic_irq_enable(EXTI2_IRQn,2,2);									//使能外部中断线 EXTI2； 设置优先级
	
	exti_interrupt_flag_clear(EXTI_2);									// 清除中断标志位


}

/************************************************************ 
 * Function :       EXTI2_IRQHandler
 * Comment  :       EXTI2的中断服务函数，触发中断后LED2翻转
************************************************************/
void EXTI2_IRQHandler(void)
{
	if(RESET != exti_interrupt_flag_get(EXTI_2))
	{
		gpio_bit_toggle(GPIOA,GPIO_PIN_5);   // 端口电平翻转
		
		exti_interrupt_flag_clear(EXTI_2);	 // 清除中断标志位
	
	}

}
