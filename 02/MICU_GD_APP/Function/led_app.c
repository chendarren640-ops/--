/* SPDX-License-Identifier: MIT */

/**
 * @file    led_app.c
 * @brief   Application-layer LED indicator driver.
 *
 * Controls six user LEDs on the GD32F4xx board:
 *   - LED_SYS  (LED1): system heartbeat, toggles at 1 Hz while the APP runs.
 *   - LED_SMP  (LED2): sample status, solid ON during auto-report (0x0302),
 *                       OFF otherwise.
 *   - LED3..LED6: unused, kept off.
 */

#include "board_defs.h"
#include "cimc_app.h"

/** Toggle period in milliseconds for the system-status LED (1 s full cycle). */
#define LED_BLINK_MS  500U

/**
 * @brief LED display task, invoked every 1 ms from the scheduler.
 */
void Led_Process(void)
{
    static uint32_t lastToggle = 0U;
    static uint8_t  initialized = 0U;
    uint32_t now = get_system_ms();

    if (initialized == 0U)
    {
        LED_SYS_SET(0);
        LED_SMP_SET(0);
        LED3_SET(0);
        LED4_SET(0);
        LED5_SET(0);
        LED6_SET(0);
        lastToggle = now;
        initialized = 1U;
    }

    /* System status LED: 1 s blink from APP entry onward. */
    if ((now - lastToggle) >= LED_BLINK_MS)
    {
        LED_SYS_TOGGLE;
        lastToggle = now;
    }

    /* Sample LED: solid while auto-reporting, off otherwise. */
    LED_SMP_SET(cimc_app_is_auto_reporting() ? 1 : 0);
}
