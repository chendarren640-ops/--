/* SPDX-License-Identifier: MIT */

/**
 * @file    flash_param.h
 * @brief   APP runtime parameter block and BootLoader parameter structure
 *          definitions for the GD32F4xx platform.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../BSP/FlashLayout/flash_layout.h"

/******************************************************************************/
/* APP runtime parameters (stored in internal Flash) */

#define PARAM_ADDR               0x08010000
#define PARAM_SIZE               4096

typedef struct
{
    uint16_t device_id;
    uint8_t  baud_code;
    uint8_t  alarm_mode;
    float    ch0_ratio;
    float    ch1_ratio;
    float    ch0_threshold;
    float    ch1_threshold;
    float    ch2_threshold;   /**< PT100 temperature threshold */
    uint8_t  upgrade_flag;
    uint8_t  reserved[3];
    uint32_t crc32;
} ParamBlock_t;

extern ParamBlock_t g_param;
void Param_Load(void);
void Param_Save(void);
void Param_SetDefaults(void);

/******************************************************************************/
/* BootLoader parameter structure */

#define FLASH_PARAM_PAGE_ADDR    0x08010000UL
#define FLASH_PARAM_PAGE_SZ      0x00001000UL
#define FLASH_PARAM_MAIN         0x08010000UL
#define FLASH_PARAM_BAK          0x08010100UL
#define FLASH_LOG_ADDR           0x08010200UL
#define FLASH_LOG_ENTRY_SZ       32UL
#define FLASH_LOG_ENTRY_CNT      32UL

#define BOOT_PARAM_MAGIC         0x5AA5C33CUL
#define BOOT_PARAM_TAIL_MAGIC    0xA5A5C3C3UL
#define BOOT_PARAM_VER           0x00010001UL

#define UPDATE_FLAG_IDLE         0x00000000UL
#define UPDATE_FLAG_PENDING      0xAA55AA55UL
#define UPDATE_FLAG_FAILED       0xDEAD0001UL
#define UPDATE_FLAG_FAST         0x55AA55AAUL

#define OTA_ERR_OK               0UL
#define OTA_ERR_PARAM            1UL
#define OTA_ERR_CACHE            2UL
#define OTA_ERR_COPY             3UL
#define OTA_ERR_APP              4UL

#define FLASH_LOG_MAGIC          0xB100B100UL
#define FLASH_LOG_EV_PARAM_RECOVER  1UL
#define FLASH_LOG_EV_UPDATE_OK      2UL
#define FLASH_LOG_EV_UPDATE_FAIL    3UL
#define FLASH_LOG_EV_JUMP_FAIL      4UL

#define BL_COPY_CHUNK            256UL

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t update_flag;
    uint32_t app_size;
    uint32_t app_crc32;
    uint32_t app1_addr;
    uint32_t app2_addr;
    uint32_t update_counter;
    uint32_t fail_counter;
    uint32_t last_error;
    uint32_t log_write_index;
    uint32_t comm_baud_code;
    uint32_t comm_device_id;
    uint32_t reserved[49];
    uint32_t param_crc32;
    uint32_t tail_magic;
} BootParam_s;

typedef struct
{
    uint32_t magic;
    uint32_t seq;
    uint32_t event_id;
    uint32_t result;
    uint32_t value0;
    uint32_t value1;
    uint32_t value2;
    uint32_t crc32;
} BootLog_s;

bool BootParam_Commit(BootParam_s *param);
