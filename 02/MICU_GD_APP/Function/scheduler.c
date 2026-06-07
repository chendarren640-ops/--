/* SPDX-License-Identifier: MIT */

/**
 * @file    scheduler.c
 * @brief   Cooperative task scheduler for the GD32F4xx application.
 *
 * Manages a static table of periodic tasks and dispatches each task when
 * its configured interval has elapsed.
 */

#include "board_defs.h"

/** Number of registered tasks in the static task table. */
uint8_t g_TaskCount;

/**
 * @brief Descriptor for a single scheduled task.
 */
typedef struct
{
    void (*pHandler)(void);
    uint32_t periodMs;
    uint32_t lastRunMs;
} Task_s;

/**
 * @brief Static task table: handler, period in milliseconds, and last run timestamp.
 *
 * Entry order determines execution priority when multiple tasks become due
 * in the same scheduler pass.
 */
static Task_s s_TaskTable[] =
{
     {Led_Process,    1,   0}
    ,{Adc_Process,    100, 0}
    ,{Display_Process, 10, 0}
    ,{Btn_Process,    5,   0}
    ,{Uart_Process,   5,   0}
    ,{AppCtrl_Poll,   50,  0}
    ,{Rtc_Process,    500, 0}
};

/**
 * @brief Initialise the scheduler by computing the task count from the
 *        static table.
 */
void Sched_Init(void)
{
    g_TaskCount = sizeof(s_TaskTable) / sizeof(Task_s);
}

/**
 * @brief Run all due tasks according to their millisecond period.
 *
 * Each task whose elapsed time since last execution meets or exceeds its
 * configured period is invoked once.  The run order is the table order.
 */
void Sched_Run(void)
{
    for (uint8_t i = 0; i < g_TaskCount; i++)
    {
        uint32_t nowTime = get_system_ms();
        if (nowTime >= s_TaskTable[i].periodMs + s_TaskTable[i].lastRunMs)
        {
            s_TaskTable[i].lastRunMs = nowTime;
            s_TaskTable[i].pHandler();
        }
    }
}
