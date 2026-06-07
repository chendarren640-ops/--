#ifndef __PT100_H
#define __PT100_H

#include "gd32f4xx.h"
#include <stdint.h>

/* 硬件参数（直接测量PT100两端电压，无放大、无偏置） */
#define PT100_EXC_CURRENT_A     0.000448f   // 实测恒流源电流 (A)
#define PT100_AMP_GAIN          1.0f        // 无放大，增益为1
#define PT100_BIAS_VOLTAGE      0.0f        // 无偏置

/* PT100传感器常数 */
#define PT100_R0                  100.0f
#define PT100_A_COEFF             3.9083e-3f
#define PT100_B_COEFF            -5.775e-7f

/* 错误及限幅 */
#define PT100_ERROR_VALUE      (-273.15f)
#define PT100_MIN_VALID_RESISTANCE  80.0f
#define PT100_MAX_VALID_RESISTANCE  214.0f

float PT100_VoltageToResistance(float adc_voltage);
float PT100_ResistanceToTemperature(float resistance);
float PT100_VoltageToTemperature(float adc_voltage);

#endif /* __PT100_H */

