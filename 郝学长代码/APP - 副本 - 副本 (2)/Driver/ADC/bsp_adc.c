/*
 * bsp_adc.c
 * ADC0 驱动 - 轮询模式（每次读取启动转换并等待结果）
 * CH0 (PC0, ADC0_IN10): 电位器采样
 * CH1 (PC1, ADC0_IN11): DAC 回读采样
 */

#include "bsp_adc.h"
#include "systick.h"
#include "gd32f4xx_adc.h"
#include "gd32f4xx_rcu.h"

void BSP_ADC_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_ADC0);

    /* PC0, PC1 模拟输入 */
    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1);

    /* ADC 复位 */
    adc_deinit();

    /* ADC 时钟分频 */
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);

    /* 禁用外部触发，软件触发模式 */
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, DISABLE);

    /* 配置 */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_resolution_config(ADC0, ADC_RESOLUTION_12B);

    /* 禁用扫描和连续模式（单次转换） */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);

    /* 使能 ADC */
    adc_enable(ADC0);
    delay_1ms(1);

    /* 校准 */
    adc_calibration_enable(ADC0);
}

/*
 * 读取指定通道的 ADC 原始值（轮询方式）
 */
static uint16_t BSP_ADC_ReadChannel(uint8_t channel)
{
    uint32_t timeout;

    /* 配置常规通道长度为 1 */
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1);

    /* 配置通道 */
    adc_routine_channel_config(ADC0, 0, channel, ADC_SAMPLETIME_480);

    /* 清除 EOC 标志 */
    adc_flag_clear(ADC0, ADC_FLAG_EOC);

    /* 软件触发转换 */
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);

    /* 等待转换完成 */
    timeout = 100000;
    while (!adc_flag_get(ADC0, ADC_FLAG_EOC) && timeout > 0)
    {
        timeout--;
    }

    if (timeout == 0)
    {
        return 0;
    }

    return (uint16_t)(ADC_RDATA(ADC0) & 0xFFFF);
}

uint16_t BSP_ADC_GetCH0Raw(void)
{
    return BSP_ADC_ReadChannel(BSP_ADC_CH0);
}

uint16_t BSP_ADC_GetCH1Raw(void)
{
    return BSP_ADC_ReadChannel(BSP_ADC_CH1);
}

float BSP_ADC_GetCH0Voltage(void)
{
    uint16_t raw = BSP_ADC_GetCH0Raw();
    return ((float)raw * BSP_ADC_REF_VOLTAGE) / BSP_ADC_RESOLUTION_12BIT;
}

float BSP_ADC_GetCH1Voltage(void)
{
    uint16_t raw = BSP_ADC_GetCH1Raw();
    return ((float)raw * BSP_ADC_REF_VOLTAGE) / BSP_ADC_RESOLUTION_12BIT;
}

uint16_t BSP_ADC_ReadRaw(uint8_t channel)
{
    return BSP_ADC_ReadChannel(channel);
}

float BSP_ADC_ReadVoltage(uint8_t channel)
{
    uint16_t raw = BSP_ADC_ReadChannel(channel);
    return ((float)raw * BSP_ADC_REF_VOLTAGE) / BSP_ADC_RESOLUTION_12BIT;
}
