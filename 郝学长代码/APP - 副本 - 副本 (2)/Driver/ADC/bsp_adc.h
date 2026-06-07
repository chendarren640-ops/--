/*
 * bsp_adc.h
 * ADC0 驱动头文件
 * CH0 (PC0, ADC0_IN10): 电位器采样
 * CH1 (PC1, ADC0_IN11): DAC 回读采样
 */

#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "gd32f4xx.h"
#include <stdint.h>

#define BSP_ADC_CH0                ADC_CHANNEL_10
#define BSP_ADC_CH1                ADC_CHANNEL_11

#define BSP_ADC_REF_VOLTAGE        3.3f
#define BSP_ADC_RESOLUTION_12BIT   4095.0f

void     BSP_ADC_Init(void);
uint16_t BSP_ADC_GetCH0Raw(void);
uint16_t BSP_ADC_GetCH1Raw(void);
float    BSP_ADC_GetCH0Voltage(void);
float    BSP_ADC_GetCH1Voltage(void);
uint16_t BSP_ADC_ReadRaw(uint8_t channel);
float    BSP_ADC_ReadVoltage(uint8_t channel);

#endif
