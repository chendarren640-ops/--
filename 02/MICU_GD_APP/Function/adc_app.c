/* SPDX-License-Identifier: MIT */

/**
 * @file    adc_app.c
 * @brief   Application-layer ADC driver for PT100 temperature sensing.
 *
 * Reads the GD30AD3344 external ADC channel 4 (AIN0 vs GND) with a
 * 6.144 V PGA reference and converts the voltage to an approximate
 * temperature in degrees Celsius.
 */

#include "board_defs.h"

extern uint16_t adc_value[2];
extern uint16_t convertarr[CONVERT_NUM];

/** Latest PT100 voltage reading (volts). */
float g_Pt100Voltage = 0.0f;

/** Latest PT100 temperature reading (degrees Celsius). */
float g_Pt100Temp = 0.0f;

/**
 * @brief Convert a PT100 voltage to an approximate temperature.
 * @param voltage  Measured voltage in volts.
 * @return         Approximate temperature in degrees Celsius.
 */
static float Pt100_VoltToTemp(float voltage)
{
    return voltage * 100.0f;
}

/**
 * @brief ADC periodic task, invoked every 100 ms from the scheduler.
 *
 * Reads the external ADC and updates the global voltage and temperature
 * variables.
 */
void Adc_Process(void)
{
    float result = 0.0f;
    result = GD30AD3344_AD_Read(GD30AD3344_Channel_4, GD30AD3344_PGA_6V144);
    g_Pt100Voltage = result;
    g_Pt100Temp = Pt100_VoltToTemp(result);
}
