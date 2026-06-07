/* SPDX-License-Identifier: MIT */

/**
 * @file    rtc_app.h
 * @brief   Public API for the application-layer RTC driver.
 */

#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/** RTC periodic task. */
void Rtc_Process(void);

#ifdef __cplusplus
}
#endif
