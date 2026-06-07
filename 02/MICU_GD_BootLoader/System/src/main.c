/* SPDX-License-Identifier: MIT */

/**
 * @file    main.c
 * @brief   BootLoader entry point
 *
 * Initializes system tick and hardware UART, then enters the bootloader
 * main loop. The bootloader determines whether to jump to the application
 * or stay in firmware-update mode.
 */

#include "board_defs.h"
#include "loader_core.h"

int main(void)
{
    Tick_Init();
    HalUart_Setup();

    Loader_Run();

    while(1)
    {
    }
}
