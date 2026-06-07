/* SPDX-License-Identifier: MIT */

/**
 * @file    oled_app.c
 * @brief   Application-layer 128x32 SSD1306 OLED display driver.
 *
 * Implements the two-line status screen required by specification 2.6:
 *   Line 1 (page 0): team registration number
 *   Line 2 (page 2): "AutoSample" while auto-reporting, otherwise "IDLE"
 *
 * The function oled_printf() is retained under its original name because it
 * is called directly from cmd_dispatch.c and cimc_app.c to paint the
 * "Bootloader" message before a warm reset into the BootLoader.
 */

#include "board_defs.h"
#include "cimc_app.h"

extern uint16_t adc_value[2];

/**
 * @brief Team registration number displayed on OLED line 1 (spec 2.6).
 *
 * Edit this to the actual competition team number before the contest.
 */
#ifndef TEAM_NUMBER
#define TEAM_NUMBER "TEAM-0000"
#endif

/**
 * @brief Print formatted text on the OLED using the 8x16 ASCII font.
 * @param x      Pixel position on the X axis, range 0-127 (8 px per char).
 * @param y      Start page on the Y axis.  The 8x16 glyph spans pages y and
 *               y+1, so on this 128x32 (4-page) panel the two text rows are
 *               y=0 (pages 0-1) and y=2 (pages 2-3), filling the screen.
 * @param format printf-style format string.
 * @param ...    Variable arguments for the format string.
 * @return       Formatted string length returned by vsnprintf().
 */
int oled_printf(uint8_t x, uint8_t y, const char *format, ...)
{
    char buffer[512];
    va_list arg;
    int len;

    va_start(arg, format);
    len = vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);

    Display_ShowStr(x, y, buffer, 16);
    return len;
}

/**
 * @brief OLED display task, invoked every 10 ms from the scheduler.
 *
 * Initialises the display on first call and updates the status line when
 * the auto-reporting state changes.
 */
void Display_Process(void)
{
    static uint8_t initialized = 0U;
    static uint8_t lastReporting = 0xFFU;
    uint8_t reporting = cimc_app_is_auto_reporting() ? 1U : 0U;

    if (initialized == 0U)
    {
        Display_Clear();
        oled_printf(0, 0, "%s", TEAM_NUMBER);
        initialized = 1U;
        lastReporting = 0xFFU;  /* force the status line to paint once */
    }

    if (reporting != lastReporting)
    {
        /* Trailing spaces pad to a fixed width so the longer string is
           fully erased on the display. */
        oled_printf(0, 2, reporting ? "AutoSample" : "IDLE      ");
        lastReporting = reporting;
    }
}
