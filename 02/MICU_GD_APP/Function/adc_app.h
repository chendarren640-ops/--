/* SPDX-License-Identifier: MIT */

/**
 * @file    adc_app.h
 * @brief   Public API for the application-layer ADC driver.
 */

#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Latest PT100 temperature reading in degrees Celsius. */
extern float g_Pt100Temp;

/** Latest PT100 voltage reading in volts. */
extern float g_Pt100Voltage;

/** ADC periodic task. */
void Adc_Process(void);

#ifdef __cplusplus
}
#endif
