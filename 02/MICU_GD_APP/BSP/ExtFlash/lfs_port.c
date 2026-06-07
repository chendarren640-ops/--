/* SPDX-License-Identifier: MIT */

/**
 * @file    lfs_port.c
 * @brief   LittleFS porting layer for GD25Qxx — block device callbacks.
 */

#include "lfs_port.h"
#include "nor_flash.h"
#include <stdio.h>

/** Static buffers required by LittleFS */
static uint8_t s_LfsReadBuf[LFS_FLASH_PAGE_SZ];
static uint8_t s_LfsProgBuf[LFS_FLASH_PAGE_SZ];
static uint8_t s_LfsLookahead[256 / 8];

/**
 * @brief   Block device read callback.
 */
static int LfsDev_Read(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, void *buffer, lfs_size_t size)
{
    (void)c;
    NorFlash_Read(buffer, (block * LFS_FLASH_SECTOR_SZ) + off, size);
    return LFS_ERR_OK;
}

/**
 * @brief   Block device program callback.
 */
static int LfsDev_Prog(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, const void *buffer, lfs_size_t size)
{
    (void)c;
    NorFlash_Write((uint8_t *)buffer, (block * LFS_FLASH_SECTOR_SZ) + off, size);
    return LFS_ERR_OK;
}

/**
 * @brief   Block device erase callback.
 */
static int LfsDev_Erase(const struct lfs_config *c, lfs_block_t block)
{
    (void)c;
    NorFlash_EraseSector(block * LFS_FLASH_SECTOR_SZ);
    return LFS_ERR_OK;
}

/**
 * @brief   Block device sync callback.
 *
 * The NorFlash driver's write functions are blocking and wait for write
 * completion, so no extra action is needed here.
 */
static int LfsDev_Sync(const struct lfs_config *c)
{
    (void)c;
    return LFS_ERR_OK;
}

/**
 * @brief   Initialize the LittleFS configuration with GD25Qxx-backed storage.
 * @param   cfg  Pointer to lfs_config to populate
 * @return  LFS_ERR_OK on success, LFS_ERR_INVAL if cfg is NULL
 */
int LfsStorage_Init(struct lfs_config *cfg)
{
    if (!cfg)
    {
        return LFS_ERR_INVAL;
    }

    cfg->context = NULL;

    /* Block device operations */
    cfg->read  = LfsDev_Read;
    cfg->prog  = LfsDev_Prog;
    cfg->erase = LfsDev_Erase;
    cfg->sync  = LfsDev_Sync;

    /* Block device configuration */
    cfg->read_size      = LFS_FLASH_PAGE_SZ;
    cfg->prog_size      = LFS_FLASH_PAGE_SZ;
    cfg->block_size     = LFS_FLASH_SECTOR_SZ;
    cfg->block_count    = LFS_BLOCK_CNT;
    cfg->cache_size     = LFS_FLASH_PAGE_SZ;
    cfg->lookahead_size = sizeof(s_LfsLookahead);
    cfg->block_cycles   = 500;

    /* Assign static buffers */
    cfg->read_buffer      = s_LfsReadBuf;
    cfg->prog_buffer      = s_LfsProgBuf;
    cfg->lookahead_buffer = s_LfsLookahead;

    cfg->name_max = 0;
    cfg->file_max = 0;
    cfg->attr_max = 0;

    return LFS_ERR_OK;
}
