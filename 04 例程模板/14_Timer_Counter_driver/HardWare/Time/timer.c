/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：timer.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/29      V0.01    original
************************************************************/

/************************* 头文件 *************************/
#include "timer.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/

uint16_t readvalue1 = 0 , readvalue2 = 0;
__IO uint32_t count = 0;

/************************ 函数定义 ************************/
void timer_gpio_init(void);
void nvic_configuration(void);
void timer_config(void);

/************************************************************
 * Function :       my_timer_init
 * Comment  :       用于初始化定时器2
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.01 original
************************************************************/
void my_timer_init(void)
{

	timer_gpio_init();
	nvic_configuration();
	timer_config();
}

/************************************************************
 * Function :       timer_gpio_init
 * Comment  :       用于初始化定时器2的GPIO引脚（脉冲输入模式）
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.01 original
************************************************************/
void timer_gpio_init(void)
{

	rcu_periph_clock_enable(RCU_GPIOB);

	/*configure PB4 (TIMER2 CH0) as alternate function*/
	gpio_mode_set(GPIOB , GPIO_MODE_AF , GPIO_PUPD_NONE , GPIO_PIN_4);
	gpio_output_options_set(GPIOB , GPIO_OTYPE_PP , GPIO_OSPEED_50MHZ , GPIO_PIN_4);

	gpio_af_set(GPIOB , GPIO_AF_2 , GPIO_PIN_4);
}

void nvic_configuration(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
	nvic_irq_enable(TIMER2_IRQn , 1 , 1);
}

/************************************************************
 * Function :       timer_config
 * Comment  :       用于配置定时器2（输入捕获模式）
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.01 original
************************************************************/
void timer_config(void)
{

	/* TIMER2 configuration: input capture mode -------------------
		the external signal is connected to TIMER2 CH0 pin (PB4)
		the rising edge is used as active edge
		the TIMER2 CH0CV is used to compute the frequency value
		触发上升沿时，TIMER2 CH0CV寄存器的值被捕获  中断标志位被设置
		------------------------------------------------------------ */
	timer_ic_parameter_struct timer_icinitpara;
	timer_parameter_struct timer_initpara;

	rcu_periph_clock_enable(RCU_TIMER2);
	rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);

	timer_deinit(TIMER2);

	/* TIMER2 configuration */
	timer_initpara.prescaler = 2400 - 1;
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = 0xFFFF - 1;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(TIMER2 , &timer_initpara);

	/* TIMER2  configuration */
	/* TIMER2 CH0 input capture configuration */
	timer_icinitpara.icpolarity = TIMER_IC_POLARITY_RISING;
	timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
	timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;
	timer_icinitpara.icfilter = 0x0;
	timer_input_capture_config(TIMER2 , TIMER_CH_0 , &timer_icinitpara);

	/* auto-reload preload enable */
	timer_auto_reload_shadow_enable(TIMER2);
	/* clear channel 0 interrupt bit */
	timer_interrupt_flag_clear(TIMER2 , TIMER_INT_CH0);
	/* channel 0 interrupt enable */
	timer_interrupt_enable(TIMER2 , TIMER_INT_CH0);

	/* TIMER2 counter enable */
	timer_enable(TIMER2);
}

/************************************************************
 * Function :       TIMER2_IRQHandler
 * Comment  :       用于处理定时器2的中断请求
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.01 original
************************************************************/
void TIMER2_IRQHandler(void)
{
	if (SET == timer_interrupt_flag_get(TIMER2 , TIMER_INT_CH0))
	{
		/* clear channel 0 interrupt bit */
		timer_interrupt_flag_clear(TIMER2 , TIMER_INT_CH0);

		count++;

	}
}

/****************************End*****************************/
