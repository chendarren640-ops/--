/* SPDX-License-Identifier: MIT */

/**
 * @file    flash_layout.h
 * @brief   Internal flash partition layout for GD32F470VET6.
 *
 * Address space is 512 KiB (0x08000000 .. 0x0807FFFF).
 */

#pragma once

#include <stdint.h>

#undef  FLASH_BASE
#define FLASH_BASE                 0x08000000UL
#define FLASH_TOTAL_SZ             0x00080000UL
#define FLASH_END                  (FLASH_BASE + FLASH_TOTAL_SZ - 1UL)
#define FLASH_PAGE_SZ              0x00001000UL

#define BOOT_BASE                  0x08000000UL
#define BOOT_SZ                    0x00010000UL
#define BOOT_END                   (BOOT_BASE + BOOT_SZ - 1UL)

#define FLASH_PARAM_BASE           0x08010000UL
#define FLASH_PARAM_SIZE           0x00001000UL

#define APP1_BASE                  0x08011000UL
#define APP1_SZ                    0x00020000UL
#define APP1_END                   (APP1_BASE + APP1_SZ - 1UL)

#define APP2_BASE                  0x08031000UL
#define APP2_SZ                    0x00020000UL
#define APP2_END                   (APP2_BASE + APP2_SZ - 1UL)

#define APP_CACHE_BASE             0x08051000UL
#define APP_CACHE_SZ               0x00020000UL
#define APP_CACHE_END              (APP_CACHE_BASE + APP_CACHE_SZ - 1UL)

/**
 * Alias -- the download/update cache region is the same as the second app slot
 * used during OTA upgrade.
 */
#define UPDATE_CACHE_BASE          APP_CACHE_BASE
#define UPDATE_CACHE_SZ            APP_CACHE_SZ

#if ((BOOT_END + 1UL) != FLASH_PARAM_BASE)
#error "BootLoader and parameter page must be contiguous"
#endif

#if ((FLASH_PARAM_BASE + FLASH_PARAM_SIZE) != APP1_BASE)
#error "Parameter page and APP1 must be contiguous"
#endif

#if (APP1_SZ > APP2_SZ)
#error "Backup region must be large enough for APP1"
#endif

#if (APP_CACHE_SZ > APP1_SZ)
#error "Download cache must not accept images larger than APP1"
#endif

#if (APP_CACHE_END > FLASH_END)
#error "Download cache exceeds internal flash"
#endif
