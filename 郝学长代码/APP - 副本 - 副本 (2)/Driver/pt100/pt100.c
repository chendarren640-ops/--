#include "pt100.h"
#include <math.h>

/**
 * @brief  将 ADC 电压转换为 PT100 电阻值（使用线性拟合公式）
 * @param  adc_voltage ADC 测量到的前端输出电压，单位 V
 * @return PT100 电阻值，单位 Ω；异常时返回 PT100_ERROR_VALUE
 */
float PT100_VoltageToResistance(float adc_voltage)
{
    float resistance;

    /* 直接使用线性拟合公式（无需偏置和增益） */
    resistance = PT100_LINEAR_INTERCEPT + PT100_LINEAR_SLOPE * adc_voltage;

    /* 简单范围检查，防止明显错误值继续参与温度计算 */
    if((resistance < PT100_MIN_VALID_RESISTANCE) ||
       (resistance > PT100_MAX_VALID_RESISTANCE))
    {
        return PT100_ERROR_VALUE;   // 超出范围返回错误值
    }

    return resistance;
}

/**
 * @brief  根据 PT100 电阻计算温度（适用于 0℃ 以上）
 * @param  resistance PT100 电阻，单位 Ω
 * @return 温度，单位 ℃；异常时返回 PT100_ERROR_VALUE
 */
float PT100_ResistanceToTemperature(float resistance)
{
    float discriminant;
    float temperature;

    if(resistance <= 0.0f)
    {
        return PT100_ERROR_VALUE;
    }

    /* 0℃ 以上 PT100 方程：R = R0 * (1 + A*T + B*T^2) */
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

/**
 * @brief  PT100 线性近似温度计算，方便调试
 * @param  resistance PT100 电阻，单位 Ω
 * @return 温度，单位 ℃
 * @note   近似公式：T = (R - 100) / 0.385
 */
float PT100_ResistanceToTemperature_Linear(float resistance)
{
    if(resistance <= 0.0f)
    {
        return PT100_ERROR_VALUE;
    }

    return (resistance - PT100_R0) / 0.385f;
}

/**
 * @brief  将 ADC 电压直接转换为温度
 */
float PT100_VoltageToTemperature(float adc_voltage)
{
    float resistance;

    resistance = PT100_VoltageToResistance(adc_voltage);

    if(resistance <= 0.0f)
    {
        return PT100_ERROR_VALUE;
    }

    return PT100_ResistanceToTemperature(resistance);
}


