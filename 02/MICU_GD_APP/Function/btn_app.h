/* SPDX-License-Identifier: MIT */

/**
 * @file    btn_app.h
 * @brief   Public API for the application-layer button driver.
 */

#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/** One-time button hardware initialisation. */
void Btn_Init(void);

/** Periodic button polling task. */
void Btn_Process(void);

#ifdef __cplusplus
}
#endif
