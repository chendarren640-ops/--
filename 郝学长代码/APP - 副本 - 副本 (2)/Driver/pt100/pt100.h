/*
 * pt100.h
 * PT100 温度传感器转换函数
 */

#ifndef __PT100_H
#define __PT100_H

#include "gd32f4xx.h"
#include <stdint.h>

/* 电压-电阻线性拟合参数 (根据实际硬件校准) */
#define PT100_LINEAR_SLOPE      (-2333.94f)
#define PT100_LINEAR_INTERCEPT  2620.3f

/* PT100 物理参数 (IEC 60751 标准) */
#define PT100_R0                  100.0f
#define PT100_A_COEFF             3.9083e-3f
#define PT100_B_COEFF            -5.775e-7f

/* 范围限制 */
#define PT100_ERROR_VALUE         (-273.15f)
#define PT100_MIN_VALID_RESISTANCE  80.0f
#define PT100_MAX_VALID_RESISTANCE  214.0f

/* 函数声明 */
float PT100_VoltageToResistance(float adc_voltage);
float PT100_ResistanceToTemperature(float resistance);
float PT100_VoltageToTemperature(float adc_voltage);
float PT100_ResistanceToTemperature_Linear(float resistance);

#endif /* __PT100_H */
