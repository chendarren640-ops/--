#ifndef __BSP_DAC_H
#define __BSP_DAC_H

#include "HeaderFiles.h"
#include "gd32f4xx.h"  


/* DAC输出通道：PA4 (DAC_OUT0) */
#define BSP_DAC_OUT0    DAC_OUT0        // PA4

/* 函数声明 */
void BSP_DAC_Init(void);
void BSP_DAC_SetVoltage(float voltage);// voltage: 0.0 ~ 3.3V

#endif /* __BSP_DAC_H */


