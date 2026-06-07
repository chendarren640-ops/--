#ifndef __ADC_DRV_H
#define __ADC_DRV_H
#include "main.h"
#define ADC_SAMPLES 8
void ADC_Init(void);
float ADC_ReadCH0(void);   /* 电位器, PC0, ADC0_CH10 */
float ADC_ReadCH1(void);   /* DAC回读, PC1, ADC0_CH11 (需跳线: PA4→PC1) */
#endif
