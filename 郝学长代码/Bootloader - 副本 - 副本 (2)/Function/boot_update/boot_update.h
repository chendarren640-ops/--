/*
 * boot_update.h
 * Bootloader 固件更新模块头文件
 */

#ifndef __BOOT_UPDATE_H
#define __BOOT_UPDATE_H

#include <stdint.h>
#include "bsp_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明 */
struct boot_frame_struct;
typedef struct boot_frame_struct boot_frame_t;

#define BOOT_UPDATE_CHUNK_SIZE        256U

uint8_t BootUpdate_Start(uint32_t fw_size, uint32_t fw_crc32, uint32_t fw_version);
uint8_t BootUpdate_WriteData(uint32_t offset, const uint8_t *data, uint16_t len);
uint8_t BootUpdate_WriteChunk(const uint8_t *data, uint16_t len);
uint8_t BootUpdate_End(void);

uint8_t BootUpdate_CopyTempToApp(uint32_t size);
uint8_t BootUpdate_CheckTempCRC(uint32_t size, uint32_t expected_crc32);
uint8_t BootUpdate_BackupApp(uint32_t size);

int boot_update_process_frame(boot_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif
