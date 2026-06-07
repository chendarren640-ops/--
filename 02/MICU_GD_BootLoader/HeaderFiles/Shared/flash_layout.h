/* SPDX-License-Identifier: MIT */

/**
 * @file    flash_layout.h
 * @brief   Internal flash memory layout for BootLoader and application
 *
 * Defines the partition boundaries for the GD32F4xx 512 KiB internal
 * flash. The layout accommodates the BootLoader image, a parameter page,
 * the active application slot (APP1), a rollback backup slot, and an
 * OTA download cache (APP2).
 *
 * APP1 is the executable image region. APP_BAK keeps the previous
 * runnable image for rollback. APP2 is kept as the protocol-compatible
 * name for the OTA download cache.
 */

#pragma once

#include <stdint.h>

/* ---- Flash device geometry ---- */

#define FLASH_BASE                 0x08000000UL
#define FLASH_TOTAL                0x00080000UL
#define FLASH_END                  (FLASH_BASE + FLASH_TOTAL - 1UL)
#define FLASH_PAGE_SZ              0x00001000UL

/* ---- BootLoader image (keep the linker scatter file inside this range) ---- */

#define BOOT_BASE                  0x08000000UL
#define BOOT_SZ                    0x00010000UL
#define BOOT_END                   (BOOT_BASE + BOOT_SZ - 1UL)

/* ---- One flash page: main parameter copy, backup copy, and compact log entries ---- */

#define PARAM_BASE                 0x08010000UL
#define PARAM_SZ                   0x00001000UL

/* ---- Active application slot (BootLoader jumps here after validation) ---- */

#define APP1_BASE                  0x08011000UL
#define APP1_SZ                    0x00020000UL
#define APP1_END                   (APP1_BASE + APP1_SZ - 1UL)

/* ---- Previous application image used for rollback after a failed upgrade ---- */

#define APP_BAK_BASE               0x08031000UL
#define APP_BAK_SZ                 0x00020000UL
#define APP_BAK_END                (APP_BAK_BASE + APP_BAK_SZ - 1UL)

/* ---- OTA download cache (APP writes new firmware here) ---- */

#define APP_CACHE_BASE             0x08051000UL
#define APP_CACHE_SZ               0x00020000UL
#define APP_CACHE_END              (APP_CACHE_BASE + APP_CACHE_SZ - 1UL)

/* ---- Backward-compatible names used by the existing OTA protocol/parameter code ---- */

#define APP2_BASE                  APP_CACHE_BASE
#define APP2_SZ                    APP_CACHE_SZ
#define APP2_END                   (APP2_BASE + APP2_SZ - 1UL)

/* ---- Compile-time layout integrity checks ---- */

#if (BOOT_END + 1UL) != PARAM_BASE
#error "BootLoader and parameter page must be contiguous"
#endif

#if (PARAM_BASE + PARAM_SZ) != APP1_BASE
#error "Parameter page and APP1 must be contiguous"
#endif

#if (APP1_SZ > APP_BAK_SZ)
#error "Backup region must be large enough for APP1"
#endif

#if (APP_CACHE_SZ > APP1_SZ)
#error "Download cache must not accept images larger than APP1"
#endif

#if APP_CACHE_END > FLASH_END
#error "Download cache exceeds internal flash"
#endif
