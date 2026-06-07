/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：timer.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/29      V0.01    original
************************************************************/

/************************* 头文件 *************************/
#include "timer.h"
#include "fwdgt.h"

/************************* 宏定义 *************************/

/************************ 变量定义 ************************/

//记录是否喂狗
uint8_t fdgt_flag = 0;
extern uint8_t key_down_flag;

/************************ 函数定义 ************************/
void nvic_config(void);

/************************************************************
 * Function :       my_timer_init
 * Comment  :       用于初始化定时器6（普通定时器功能 1s中断一次）
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void my_timer_init(void)
{
	nvic_config();

	rcu_periph_clock_enable(RCU_TIMER6);
	rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);

	timer_deinit(TIMER6);

	timer_parameter_struct timer_initpara;

	timer_initpara.prescaler = 240 - 1;
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = 1000000 - 1;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;

	timer_init(TIMER6 , &timer_initpara);

	timer_interrupt_enable(TIMER6 , TIMER_INT_FLAG_UP);

	timer_enable(TIMER6);
}
/************************************************************
 * Function :       nvic_config
 * Comment  :       用于初始化NVIC（中断向量控制器）
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void nvic_config(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
	nvic_irq_enable(TIMER6_IRQn , 1 , 0);
}

/************************************************************
 * Function :       TIMER6_IRQHandler
 * Comment  :       用于处理定时器6中断（普通定时器功能 1s中断一次）
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void TIMER6_IRQHandler(void)
{

	if (SET == timer_interrupt_flag_get(TIMER6 , TIMER_INT_FLAG_UP))
	{
		timer_interrupt_flag_clear(TIMER6 , TIMER_INT_FLAG_UP);
		if (key_down_flag)
		{
			my_fwdgt_feed();
			fdgt_flag = 1;
		}
	}

}

/****************************End*****************************/
