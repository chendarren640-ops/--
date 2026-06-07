/* SPDX-License-Identifier: MIT */

/**
 * @file    scheduler.h
 * @brief   Public API for the cooperative task scheduler.
 */

#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Number of tasks registered in the static task table (set by Sched_Init). */
extern uint8_t g_TaskCount;

void Sched_Init(void);
void Sched_Run(void);

#ifdef __cplusplus
}
#endif
