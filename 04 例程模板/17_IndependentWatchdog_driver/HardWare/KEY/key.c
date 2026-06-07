/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：adc.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/29      V0.01    original
************************************************************/

/************************* 头文件 *************************/
#include "key.h"
#include "systick.h"

/************************* 宏定义 *************************/
#define ENABLE_KEY_EXTI 1

/************************ 变量定义 ************************/
uint8_t key_down_flag = 0;

/************************ 函数定义 ************************/

/************************************************************
 * Function :       KEY_Init
 * Comment  :       用于初始化按键
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void KEY_Init(void)
{
	// 开启外设时钟
	rcu_periph_clock_enable(KEY_RCU);
	// 设置引脚模式
	gpio_mode_set(KEY_PORT , GPIO_MODE_INPUT , GPIO_PUPD_PULLUP , KEY4);

	rcu_periph_clock_enable(RCU_SYSCFG);

	// 将指定的GPIO端口引脚映射到对应的EXTI外部中断线上
	syscfg_exti_line_config(EXTI_SOURCE_GPIOE , EXTI_SOURCE_PIN4);

	// 下降沿触发
	exti_init(EXTI_4 , EXTI_INTERRUPT , EXTI_TRIG_FALLING);

	nvic_irq_enable(EXTI4_IRQn , 3 , 0);

	// 清除之前的中断标志位
	exti_interrupt_flag_clear(EXTI_4);
}

/************************************************************
 * Function :       EXTI4_IRQHandler
 * Comment  :       用于处理按键4的中断
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void EXTI4_IRQHandler(void)
{
	if (exti_interrupt_flag_get(EXTI_4) != RESET)
	{
		printf("pin_4 pass down\r\n");
		// 处理按键4的中断
		key_down_flag = !key_down_flag;
		if (key_down_flag)
		{
			printf("开始喂狗\r\n");
		}
		else
		{
			printf("停止喂狗\r\n");
		}
		// 清除按键4的中断
		exti_interrupt_flag_clear(EXTI_4);
	}
}

/****************************End*****************************/
