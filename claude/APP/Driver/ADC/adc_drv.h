#ifndef __ADC_DRV_H
#define __ADC_DRV_H
#include "main.h"
void ADC_Init(void);
float ADC_ReadCH0(void);   /* 电位器, PC0, ADC0_CH10 */
float ADC_ReadCH1(void);   /* DAC回读, PC1, ADC0_CH11 */
#endif
