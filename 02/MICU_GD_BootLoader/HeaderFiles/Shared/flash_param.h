/* SPDX-License-Identifier: MIT */

/**
 * @file    flash_param.h
 * @brief   Persistent BootLoader parameter structures and constants
 *
 * Defines the on-flash parameter page layout, the C structures for
 * the main parameter block and the circular event log, and the
 * commit function used by both BootLoader and application.
 *
 * Parameter page layout:
 *   0x08010000  main BootParam_s copy
 *   0x08010100  backup BootParam_s copy
 *   0x08010200  ring log entries
 *
 * The whole page is rewritten when parameters change because GD32
 * internal flash erases by page. Main and backup copies let the
 * BootLoader recover if one copy is incomplete or corrupted.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bl_partition.h"

/* ---- Flash address and sizing constants ---- */

#define FLASH_PARAM_BASE           0x08010000UL
#define FLASH_PARAM_SIZE           0x00001000UL
#define FLASH_PARAM_MAIN           0x08010000UL
#define FLASH_PARAM_BAK            0x08010100UL
#define FLASH_LOG_BASE             0x08010200UL
#define FLASH_LOG_ENTRY_SZ         32UL
#define FLASH_LOG_ENTRY_CNT        32UL

/* ---- Magic and version constants ---- */

#define FLASH_PARAM_MAGIC          0x5AA5C33CUL
#define FLASH_PARAM_TAIL           0xA5A5C3C3UL
#define FLASH_PARAM_VER            0x00010001UL

/*
 * The application sets UPDATE_FLAG_PENDING after writing a complete image
 * to the download cache. The BootLoader owns the transition back to IDLE
 * or FAILED after validation and copy.
 */
#define UPDATE_FLAG_IDLE           0x00000000UL
#define UPDATE_FLAG_PENDING        0xAA55AA55UL
#define UPDATE_FLAG_FAILED         0xDEAD0001UL
#define UPDATE_FLAG_FAST           0x55AA55AAUL

/* ---- Error codes ---- */

#define OTA_ERR_OK                 0UL
#define OTA_ERR_PARAM              1UL
#define OTA_ERR_CACHE              2UL
#define OTA_ERR_COPY               3UL
#define OTA_ERR_APP                4UL

/* ---- Log constants ---- */

#define LOG_MAGIC                  0xB100B100UL
#define LOG_EVT_PARAM_RECOVER      1UL
#define LOG_EVT_UPDATE_OK          2UL
#define LOG_EVT_UPDATE_FAIL        3UL
#define LOG_EVT_JUMP_FAIL          4UL

/* ---- Small stack buffer used while copying flash regions ---- */

#define FLASH_COPY_CHUNK           256UL

/* ---- Persistent parameter block shared between APP and BootLoader ---- */

/**
 * @brief  Persistent state shared between APP and BootLoader.
 *
 * CRC is computed over all preceding fields; param_crc32 is excluded
 * from the CRC calculation.
 */
typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t updFlag;
    uint32_t appSize;
    uint32_t appCrc;
    uint32_t app1Addr;
    uint32_t app2Addr;
    uint32_t updCnt;
    uint32_t failCnt;
    uint32_t lastErr;
    uint32_t logIdx;
    uint32_t baudCode;
    uint32_t devId;
    uint32_t reserved[49];
    uint32_t crc32;
    uint32_t tailMagic;
} BootParam_s;

/* ---- Fixed-size log record stored in the parameter page ring area ---- */

/**
 * @brief  Fixed-size log entry in the parameter page circular log.
 */
typedef struct
{
    uint32_t magic;
    uint32_t seq;
    uint32_t eventId;
    uint32_t result;
    uint32_t value0;
    uint32_t value1;
    uint32_t value2;
    uint32_t crc32;
} BootLog_s;

/* ---- API ---- */

/**
 * @brief  Commit a parameter block to flash (main and backup copies).
 *
 * The function validates the struct, computes the CRC, and writes both
 * the main and the backup copy in the dedicated parameter page.
 *
 * @param[in,out] param  pointer to the parameter block to commit
 * @return               true on success, false on validation or flash error
 */
bool BootParam_Commit(BootParam_s *param);
