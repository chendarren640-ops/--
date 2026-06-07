/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：timer.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/29     V0.01    original
************************************************************/

/************************* 头文件 *************************/
#include "timer.h"

/************************* 宏定义 *************************/

/************************ 变量定义 ************************/

/************************ 函数定义 ************************/
void timer_gpio_init(void);
void timer_config(void);

/************************************************************
 * Function :       my_timer_init
 * Comment  :       用于初始化定时器1（PWM模式）
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void my_timer_init(void)
{

    timer_gpio_init();

    timer_config();
}

/************************************************************
 * Function :       timer_gpio_init
 * Comment  :       用于初始化定时器1的GPIO引脚（脉冲输出模式）
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void timer_gpio_init(void) {

    rcu_periph_clock_enable(RCU_GPIOA);

    /*configure PA1(TIMER1 CH1) as alternate function*/

    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_1);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);

    gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_1);
}

/************************************************************
 * Function :       timer_config
 * Comment  :       用于配置定时器1（PWM模式） 默认占空比50% 1ms  高电平  1ms 低电平
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void timer_config(void) {

    /*
        1ms 一次溢出
    */

    timer_oc_parameter_struct timer_ocinitpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);

    timer_deinit(TIMER1);

    /* TIMER1 configuration */
    timer_initpara.prescaler = 24000 - 1;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = TIMER1_PERIOD - 1;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1, &timer_initpara);

    /* auto-reload preload disable */
    timer_auto_reload_shadow_disable(TIMER1);

    /* CH1 configuration in OC PWM mode */
    timer_ocinitpara.ocpolarity = TIMER_OC_POLARITY_HIGH;
    timer_ocinitpara.outputstate = TIMER_CCX_ENABLE;
    timer_ocinitpara.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
    timer_ocinitpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocinitpara.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
    timer_ocinitpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

    timer_channel_output_config(TIMER1, TIMER_CH_1, &timer_ocinitpara);

    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1, TIMER1_PERIOD / 2 - 1);
    timer_channel_output_mode_config(TIMER1, TIMER_CH_1, TIMER_OC_MODE_PWM1);
    timer_channel_output_shadow_config(TIMER1, TIMER_CH_1, TIMER_OC_SHADOW_DISABLE);

   timer_enable(TIMER1); 
}

/****************************End*****************************/
