/* SPDX-License-Identifier: MIT */

/**
 * @file    lfs_port.h
 * @brief   LittleFS storage port for GD25Qxx NOR flash.
 */

#pragma once

#include "lfs.h"

/** Flash parameters — adjust for the actual NOR flash model */
#define LFS_FLASH_TOTAL_SZ    (2 * 1024 * 1024)  /**< GD25Q16: 16 Mbit = 2 MiB */
#define LFS_FLASH_SECTOR_SZ   (4 * 1024)         /**< 4 KiB sector size */
#define LFS_FLASH_PAGE_SZ     (256)              /**< 256 B page size */

#define LFS_BLOCK_CNT         (LFS_FLASH_TOTAL_SZ / LFS_FLASH_SECTOR_SZ)

/**
 * @brief   Configure the LittleFS block device struct.
 * @param   cfg  Pointer to lfs_config to populate
 * @return  LFS_ERR_OK on success
 */
int LfsStorage_Init(struct lfs_config *cfg);
