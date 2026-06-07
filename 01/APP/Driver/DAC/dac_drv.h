#ifndef __DAC_DRV_H
#define __DAC_DRV_H
#include "main.h"
void DAC_Init(void);             /* PA4 (DAC_OUT0), 需跳线至 PC1 回读 */
void DAC_SetVoltage(float v);    /* 0~3.3V */
void DAC_SetValue(uint16_t v);   /* 0~4095 (12-bit) */
#endif
