/*
 * app_sample.c
 * 数据采样模块
 * CH0: 电位器 (内部ADC, PC0) -> 经变比后上报
 * CH1: DAC 回读 (内部ADC, PC1) -> 经变比后上报
 * CH2: PT100 温度 (外部ADC GD30AD3340) -> 实际温度值
 */

#include "app_sample.h"
#include "bsp_adc.h"
#include "gd30ad3340.h"
#include "pt100.h"
#include "app_param.h"
#include <string.h>

#define PT100_REF_RESISTOR  1000.0f
#define PT100_REF_VOLTAGE   3.3f

static app_sample_t s_latest_sample;

static float APP_Read_PT100_Temp(void)
{
    int16_t raw;
    float adc_voltage;
    float resistance;
    float temperature;

    if (GD30AD3340_ReadAIN0Default(&raw, &adc_voltage) != 0)
    {
        return -1.0f;
    }

    if (adc_voltage >= PT100_REF_VOLTAGE)
    {
        return -1.0f;
    }
    resistance = (adc_voltage * PT100_REF_RESISTOR) / (PT100_REF_VOLTAGE - adc_voltage);

    temperature = PT100_ResistanceToTemperature_Linear(resistance);

    return temperature;
}

void APP_Sample_Init(void)
{
    memset(&s_latest_sample, 0, sizeof(s_latest_sample));
    APP_Sample_Update();
}

void APP_Sample_Update(void)
{
    float temp;

    s_latest_sample.ch0_raw = BSP_ADC_GetCH0Raw();
    s_latest_sample.ch0_voltage = BSP_ADC_GetCH0Voltage();
    s_latest_sample.ch0_value = s_latest_sample.ch0_voltage * g_app_param.ch0_ratio;

    s_latest_sample.ch1_raw = BSP_ADC_GetCH1Raw();
    s_latest_sample.ch1_voltage = BSP_ADC_GetCH1Voltage();
    s_latest_sample.ch1_value = s_latest_sample.ch1_voltage * g_app_param.ch1_ratio;

    temp = APP_Read_PT100_Temp();
    s_latest_sample.ch2_temp = temp;
}

void APP_Sample_GetLatest(app_sample_t *sample)
{
    if (sample == NULL) return;
    memcpy(sample, &s_latest_sample, sizeof(app_sample_t));
}
