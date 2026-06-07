/* SPDX-License-Identifier: MIT */

/**
 * @file    loader_core.h
 * @brief   BootLoader core entry-point declarations
 */

#pragma once

/**
 * @brief  Main bootloader state machine entry point
 *
 * Checks the persistent parameter page for an update request,
 * validates and copies firmware images, and either jumps to the
 * application or stays in update mode.
 */
void Loader_Run(void);

/**
 * @brief  Dump the boot event log over the debug UART
 *
 * Iterates through the circular log entries in the parameter
 * flash page and prints each one in a human-readable format
 * on the configured debug USART.
 */
void Loader_LogDumpUart(void);
