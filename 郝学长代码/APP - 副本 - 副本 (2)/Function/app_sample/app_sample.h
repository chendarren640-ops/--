/*
 * app_sample.h
 * 数据采样模块头文件
 * CH0: 电位器 (内部ADC, PC0)
 * CH1: DAC 回读 (内部ADC, PC1)
 * CH2: PT100 温度 (外部ADC GD30AD3340)
 */

#ifndef __APP_SAMPLE_H__
#define __APP_SAMPLE_H__

#include <stdint.h>

/**
 * 采样数据结构体
 * CH0 = 电位器电压 × 变比
 * CH1 = DAC 回读电压 × 变比
 * CH2 = PT100 温度值 (摄氏度)
 */
typedef struct
{
    uint16_t ch0_raw;        /* CH0 内部ADC原始值 (12位) */
    float    ch0_voltage;    /* CH0 电压值 (V) */
    float    ch0_value;      /* CH0 输出值 (= ch0_voltage * ch0_ratio) */

    uint16_t ch1_raw;        /* CH1 内部ADC原始值 (12位), DAC回读 */
    float    ch1_voltage;    /* CH1 电压值 (V) */
    float    ch1_value;      /* CH1 输出值 (= ch1_voltage * ch1_ratio) */

    float    ch2_temp;       /* CH2 PT100 温度值 (摄氏度) */
} app_sample_t;

/* 采样周期 (毫秒) */
#define APP_SAMPLE_PERIOD_MS    200U

void APP_Sample_Init(void);
void APP_Sample_Update(void);
void APP_Sample_GetLatest(app_sample_t *sample);

#endif /* __APP_SAMPLE_H__ */
