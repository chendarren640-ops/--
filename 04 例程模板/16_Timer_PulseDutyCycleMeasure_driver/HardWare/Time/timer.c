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

uint32_t ic1value = 0, ic2value = 0;
__IO uint32_t dutycycle = 0;

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
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void my_timer_init(void)
{

    timer_gpio_init();
    nvic_configuration();
    timer_config();
}

/************************************************************
 * Function :       timer_gpio_init
 * Comment  :       用于初始化定时器2的GPIO引脚
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void timer_gpio_init(void) {

    rcu_periph_clock_enable(RCU_GPIOB);

    /*configure PB4 (TIMER2 CH0) as alternate function*/
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);

    gpio_af_set(GPIOB, GPIO_AF_2, GPIO_PIN_4);
}

/************************************************************
 * Function :       nvic_configuration
 * Comment  :       用于配置定时器2的中断
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void nvic_configuration(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
    nvic_irq_enable(TIMER2_IRQn, 1, 1);
}

/************************************************************
 * Function :       timer_config
 * Comment  :       用于配置定时器2
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void timer_config(void) {

    /* TIMER2 configuration: PWM input mode ------------------------
        the external signal is connected to TIMER2 CH0 pin(PB4)
        the rising edge is used as active edge
        the TIMER2 CH0CV is used to compute the frequency value
        the TIMER2 CH1CV is used to compute the duty cycle value
     ------------------------------------------------------------ */
    timer_ic_parameter_struct timer_icinitpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER2);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);

    timer_deinit(TIMER2);

    /* TIMER2 configuration */
    timer_initpara.prescaler = 240 - 1;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = 65535;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER2, &timer_initpara);

    /* TIMER2 configuration */
    /* TIMER2 CH0 PWM input capture configuration */
    timer_icinitpara.icpolarity = TIMER_IC_POLARITY_RISING;
    timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
    timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;
    timer_icinitpara.icfilter = 0x0;
    //!该函数源码会自动配置对应的互补通道
    timer_input_pwm_capture_config(TIMER2, TIMER_CH_0, &timer_icinitpara);

    /* slave mode selection: TIMER2 */
    timer_input_trigger_source_select(TIMER2, TIMER_SMCFG_TRGSEL_CI0FE0);
    timer_slave_mode_select(TIMER2, TIMER_SLAVE_MODE_RESTART);

    /* select the master slave mode */
    timer_master_slave_mode_config(TIMER2, TIMER_MASTER_SLAVE_MODE_ENABLE);

    /* auto-reload preload enable
        period的新值会等当前计数周期结束（溢出后）再生效，保证每个计数周期的长度是完整、一致的。
    */
    timer_auto_reload_shadow_enable(TIMER2);
    /* clear channel 0 interrupt bit */
    timer_interrupt_flag_clear(TIMER2, TIMER_INT_CH0);
    /* channel 0 interrupt enable */
    timer_interrupt_enable(TIMER2, TIMER_INT_CH0);

    /* TIMER2 counter enable */
    timer_enable(TIMER2);
}

/************************************************************
 * Function :       TIMER2_IRQHandler
 * Comment  :       用于处理定时器2通道0中断，计算占空比
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void TIMER2_IRQHandler(void)
{
    if (SET == timer_interrupt_flag_get(TIMER2, TIMER_INT_CH0)) {
        /* clear channel 0 interrupt bit */
        timer_interrupt_flag_clear(TIMER2, TIMER_INT_CH0);
        /* read channel 0 capture value */
        ic1value = timer_channel_capture_value_register_read(TIMER2, TIMER_CH_0) + 1;

        if (0 != ic1value) {
            /* read channel 1 capture value */
            //计算的是高电平时间的持续时间
            ic2value = timer_channel_capture_value_register_read(TIMER2, TIMER_CH_1) + 1;

            /* calculate the duty cycle value */
            dutycycle = ((ic2value * 100) % ic1value) == 0 ? (ic2value * 100) / ic1value : (ic2value * 100) / ic1value + 1;

        }
        else {
            dutycycle = 0;
        }
    }

}

/****************************End*****************************/
