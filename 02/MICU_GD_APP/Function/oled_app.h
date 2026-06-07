/* SPDX-License-Identifier: MIT */

/**
 * @file    oled_app.h
 * @brief   Public API for the application-layer OLED display driver.
 */

#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Print formatted text on the OLED.
 * @note  Retained under its original name for backward compatibility with
 *        cmd_dispatch.c and cimc_app.c.
 */
int oled_printf(uint8_t x, uint8_t y, const char *format, ...);

/** Periodic display update task. */
void Display_Process(void);

#ifdef __cplusplus
}
#endif
