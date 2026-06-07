#include "pt100.h"
#include <math.h>

float PT100_VoltageToResistance(float adc_voltage)
{
    float v_pt100;
    float resistance;

    if(PT100_EXC_CURRENT_A <= 0.0f || PT100_AMP_GAIN <= 0.0f)
    {
        return PT100_ERROR_VALUE;
    }

    /* 公式：Vout = I*G*R + Vbias  =>  R = (Vout - Vbias) / (I*G) */
    v_pt100 = (adc_voltage - PT100_BIAS_VOLTAGE) / PT100_AMP_GAIN;

    if(v_pt100 < 0.0f)
    {
        return PT100_ERROR_VALUE;
    }

    resistance = v_pt100 / PT100_EXC_CURRENT_A;

    /* 范围检查（超出不拦截，仅留注释供参考） */
    // if(resistance < PT100_MIN_VALID_RESISTANCE || resistance > PT100_MAX_VALID_RESISTANCE)
    //     return PT100_ERROR_VALUE;

    return resistance;
}

float PT100_ResistanceToTemperature(float resistance)
{
    float discriminant;
    float temperature;

    if(resistance <= 0.0f)
    {
        return PT100_ERROR_VALUE;
    }

    discriminant = PT100_A_COEFF * PT100_A_COEFF -
                   4.0f * PT100_B_COEFF * (1.0f - resistance / PT100_R0);

    if(discriminant < 0.0f)
    {
        return PT100_ERROR_VALUE;
    }

    temperature = (-PT100_A_COEFF + sqrtf(discriminant)) /
                  (2.0f * PT100_B_COEFF);

    return temperature;
}

/*
    线性近似函数（保留，可选）
*/
float PT100_ResistanceToTemperature_Linear(float resistance)
{
    if(resistance <= 0.0f)
    {
        return PT100_ERROR_VALUE;
    }
    return (resistance - PT100_R0) / 0.385f;
}

float PT100_VoltageToTemperature(float adc_voltage)
{
    float resistance = PT100_VoltageToResistance(adc_voltage);
    if(resistance <= 0.0f)
    {
        return PT100_ERROR_VALUE;
    }
    return PT100_ResistanceToTemperature(resistance);
}