/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：key.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2026/1/30     V0.01    original
************************************************************/

/************************* 头文件 *************************/

#include "key.h"
#include "LED.h"

/************************ 全局变量定义 ************************/


/************************************************************
 * Function :       KEY_Init
 * Comment  :       用于初始化按键端口
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-01-30 V0.1 original
************************************************************/

void KEY_Init(void)
{

	rcu_periph_clock_enable(RCU_SYSCFG);    // 初始化SYSCFG时钟

	rcu_periph_clock_enable(RCU_GPIOE);    // 初始化GPIO_E总线时钟

	//初始化按键端口
	gpio_mode_set(GPIOE , GPIO_MODE_INPUT , GPIO_PUPD_PULLUP , GPIO_PIN_3 | GPIO_PIN_4);   			// GPIO模式设置为输入，上拉	

	//!配置中断
	/* enable and set key EXTI interrupt to the lowest priority */
	nvic_irq_enable(EXTI3_IRQn , 2U , 0U);

	/* connect key EXTI line to key GPIO pin */
	syscfg_exti_line_config(EXTI_SOURCE_GPIOE , EXTI_SOURCE_PIN3);

	/* configure key EXTI line */
	exti_init(EXTI_3 , EXTI_INTERRUPT , EXTI_TRIG_FALLING);
	exti_interrupt_flag_clear(EXTI_3);
}

/************************************************************
 * Function :       Key_Toggle_LED
 * Comment  :       用于切换 LED 状态
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-01-30 V0.1 original
************************************************************/
void Key_Toggle_LED(void)
{
	if (gpio_input_bit_get(GPIOE , GPIO_PIN_4) == RESET)
	{
		delay_1ms(20); // 消抖
		LED_Toggle(GPIOA , GPIO_PIN_5);
		delay_1ms(10); // 消抖
	}

}

/************************************************************
 * Function :       EXTI3_IRQHandler
 * Comment  :       用于处理按键中断事件
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-01-30 V0.1 original
************************************************************/
void EXTI3_IRQHandler(void)
{
	if (RESET != exti_interrupt_flag_get(EXTI_3))
	{
		// delay_1ms(20); // 消抖
		exti_interrupt_flag_clear(EXTI_3);
		
		// 处理按键中断事件
		LED_Toggle(GPIOA , GPIO_PIN_4);
	}
}



/****************************End*****************************/

