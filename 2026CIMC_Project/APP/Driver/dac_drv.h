#ifndef __DAC_DRV_H
#define __DAC_DRV_H
#include "main.h"
void DAC_Init(void);
void DAC_SetVoltage(float v);   /* 0~3.3V */
void DAC_SetValue(uint16_t v);  /* 0~4095 */
#endif
