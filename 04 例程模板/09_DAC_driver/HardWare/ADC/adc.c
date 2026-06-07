/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：adc.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/31     V0.01    original
************************************************************/


/************************* 头文件 *************************/
#include "adc.h"
#include "dac.h"
//! 此时只跑了一个ADC,不需要扫描

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/


/************************ 函数定义 ************************/
void __ADC_Init_GPIO(void);
void __ADC_Init(void);
/************************************************************
 * Function :       ADC_Init
 * Comment  :       用于初始化ADC
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-31 V0.01 original
************************************************************/
void ADC_Init(void)
{
    __ADC_Init_GPIO();
    __ADC_Init();
    my_dac_init();
}

/************************************************************
 * Function :       __ADC_Init_GPIO
 * Comment  :       用于初始化ADC GPIO
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-31 V0.01 original
************************************************************/
void __ADC_Init_GPIO(void)
{
    // 开启ADC_GPIO时钟
    rcu_periph_clock_enable(ADC_GPIO_RCU);
    // 开启ADC时钟
    rcu_periph_clock_enable(ADC_RCU);
    // 设置ADC GPIO引脚为模拟输入模式
    gpio_mode_set(ADC_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_GPIO_PIN);

}
/************************************************************
 * Function :       __ADC_Init
 * Comment  :       用于初始化ADC
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-31 V0.01 original
************************************************************/
void __ADC_Init(void)
{
    // 复位ADC
    adc_deinit();
    // 配置ADC时钟   8分频  240/8 = 30MHz 采集频率 30MHZ ？？？这么快？？
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    // ADC模式配置   独立模式下
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    // ADC不使用扫描模式
    adc_special_function_config(ADCX, ADC_SCAN_MODE, DISABLE);
    // ADC启用连续模式
    adc_special_function_config(ADCX, ADC_CONTINUOUS_MODE, ENABLE);
    // ADC数据对齐配置(右对齐)
    adc_data_alignment_config(ADCX, ADC_DATAALIGN_RIGHT);
    // ADC 通道长度配置(使用常规)
    adc_channel_length_config(ADCX, ADC_ROUTINE_CHANNEL, 1);
    // ADC常规通道配置
    adc_routine_channel_config(ADCX, 0, ADC_CHANNEL, ADC_SAMPLETIME_56);
    // ADC 触发配置 //!先试用手动触发 采集电压的不需要自动触发
    //! 下面是常规触发
    // adc_external_trigger_source_config(ADCX, ADC_ROUTINE_CHANNEL, ADC_EXTTRIG_ROUTINE_NONE);
    adc_external_trigger_config(ADCX, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);

    // 开启adc接口
    adc_enable(ADCX);
    // 等待1ms
    delay_1ms(1);
    // ADC校准和复位ADC校准
    adc_calibration_enable(ADCX);
}
/************************************************************
 * Function :       ADC_Read_Register
 * Comment  :       用于读取ADC寄存器值
 * Parameter:       null
 * Return   :       uint16_t  ADC寄存器值
 * Author   :       Jialei Zhao
 * Date     :       2025-12-31 V0.01 original
************************************************************/
uint16_t ADC_Read_Register(void)
{
    // 启用转化
    adc_software_trigger_enable(ADCX, ADC_ROUTINE_CHANNEL);
    // 等待转化完成
    while (!adc_flag_get(ADCX, ADC_FLAG_EOC))
        ;
    // 清除EOC标志
    adc_flag_clear(ADCX, ADC_FLAG_EOC);
    uint16_t adc_value = adc_routine_data_read(ADCX);

	// 转换为DAC输出值
    /* set DAC output data */
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, adc_value);
    /* enable DAC software trigger */
    dac_software_trigger_enable(DAC0, DAC_OUT0);
    // 返回转换结果
    return adc_value;
}

/****************************End*****************************/
