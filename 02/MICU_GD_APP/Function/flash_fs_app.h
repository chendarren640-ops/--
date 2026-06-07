/* SPDX-License-Identifier: MIT */

/**
 * @file    flash_fs_app.h
 * @brief   Public API for the external SPI flash littlefs filesystem.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialise and mount the littlefs filesystem. */
int FlashFs_Init(void);

/** Run a write/read/verify self-test on the filesystem. */
void FlashFs_Test(void);

/** Read a file from the filesystem into a buffer. */
int FlashFs_Read(const char *path, void *buffer, uint32_t size);

/** Write data from a buffer to a file on the filesystem. */
int FlashFs_Write(const char *path, const void *buffer, uint32_t size);

#ifdef __cplusplus
}
#endif
