/* SPDX-License-Identifier: MIT */

/**
 * @file    rtc_app.c
 * @brief   Application-layer RTC driver.
 *
 * Periodically reads the current date and time from the on-chip RTC
 * peripheral into the shared rtc_parameter_struct.
 */

#include "board_defs.h"

/** Shared RTC parameter structure (defined in the BSP layer). */
extern rtc_parameter_struct rtc_initpara;

/**
 * @brief RTC periodic task, invoked every 500 ms from the scheduler.
 *
 * Reads the current time from the RTC hardware into the shared parameter
 * structure.
 */
void Rtc_Process(void)
{
    rtc_current_time_get(&rtc_initpara);
}
