/*
 * boot_update.c
 * Bootloader 固件更新模块
 */

#include "boot_update.h"
#include "boot_flag.h"
#include "protocol_crc.h"
#include "boot_protocol.h"
#include <string.h>

static uint32_t s_fw_size = 0;
static uint32_t s_fw_crc32 = 0xFFFFFFFFUL;
static uint32_t s_fw_version = 0;
static uint8_t  s_update_started = 0;
static uint32_t s_write_offset = 0;

static uint8_t s_buf[BOOT_UPDATE_CHUNK_SIZE];

/*
 * boot_update_process_frame - 处理升级相关帧
 * 返回值: 0=普通, 1=进入升级模式, 2=跳转APP
 */
int boot_update_process_frame(boot_frame_t *frame)
{
    if(frame == 0) return 0;
    return BootProtocol_ProcessFrameEx(frame);
}

/*
 * BootUpdate_Start - 开始固件更新
 * 擦除固件暂存区, 准备接收数据
 * 如果 fw_size=0, 则使用整个暂存区大小
 */
uint8_t BootUpdate_Start(uint32_t fw_size, uint32_t fw_crc32, uint32_t fw_version)
{
    s_fw_size = fw_size;
    s_fw_crc32 = fw_crc32;
    s_fw_version = fw_version;
    s_write_offset = 0;

    /* 擦除固件暂存区 */
    if(!BSP_FLASH_Erase(FW_TEMP_ADDR, FW_TEMP_SIZE))
    {
        return 0;
    }

    if(fw_size > 0)
    {
        BootFlag_SetUpdateInfo(fw_size, fw_crc32, fw_version);
    }

    s_update_started = 1;
    return 1;
}

/*
 * BootUpdate_WriteData - 写入一帧固件数据到暂存区
 */
uint8_t BootUpdate_WriteData(uint32_t offset, const uint8_t *data, uint16_t len)
{
    if(!s_update_started) return 0;
    if(data == 0 || len == 0) return 0;
    if((offset + len) > FW_TEMP_SIZE) return 0;

    return BSP_FLASH_Write(FW_TEMP_ADDR + offset, data, len);
}

/*
 * BootUpdate_WriteChunk - 直接写入一帧原始数据 (用于接收赛题的原始固件帧)
 */
uint8_t BootUpdate_WriteChunk(const uint8_t *data, uint16_t len)
{
    uint8_t ret;
    if(!s_update_started) return 0;
    if(data == 0 || len == 0) return 0;
    if((s_write_offset + len) > FW_TEMP_SIZE) return 0;

    ret = BSP_FLASH_Write(FW_TEMP_ADDR + s_write_offset, data, len);
    if(ret) s_write_offset += len;
    return ret;
}

uint8_t BootUpdate_CheckTempCRC(uint32_t size, uint32_t expected_crc32)
{
    uint32_t offset = 0;
    uint32_t remain = size;
    uint32_t read_len;
    uint32_t crc;

    if(size == 0 || size > FW_TEMP_SIZE) return 0;

    crc = 0xFFFFFFFFUL;

    while(remain > 0)
    {
        read_len = (remain > BOOT_UPDATE_CHUNK_SIZE) ? BOOT_UPDATE_CHUNK_SIZE : remain;
        BSP_FLASH_Read(FW_TEMP_ADDR + offset, s_buf, read_len);
        crc = Protocol_CRC32_Update(crc, s_buf, read_len);
        offset += read_len;
        remain -= read_len;
    }

    crc = Protocol_CRC32_Finish(crc);
    return (crc == expected_crc32) ? 1 : 0;
}

uint8_t BootUpdate_CopyTempToApp(uint32_t size)
{
    uint32_t offset = 0;
    uint32_t remain = size;
    uint32_t copy_len;

    if(size == 0 || size > APP_SIZE) return 0;

    if(!BSP_FLASH_Erase(APP_START_ADDR, APP_SIZE))
        return 0;

    while(remain > 0)
    {
        copy_len = (remain > BOOT_UPDATE_CHUNK_SIZE) ? BOOT_UPDATE_CHUNK_SIZE : remain;
        BSP_FLASH_Read(FW_TEMP_ADDR + offset, s_buf, copy_len);
        if(!BSP_FLASH_Write(APP_START_ADDR + offset, s_buf, copy_len))
            return 0;
        offset += copy_len;
        remain -= copy_len;
    }

    return BSP_FLASH_IsAppValid(APP_START_ADDR);
}

uint8_t BootUpdate_End(void)
{
    uint32_t actual_size;

    if(!s_update_started) return 0;

    /* 确定固件实际大小 */
    actual_size = (s_fw_size > 0) ? s_fw_size : s_write_offset;
    if(actual_size == 0) return 0;

    /* 如果有 CRC, 验证 */
    if(s_fw_crc32 != 0xFFFFFFFFUL && s_fw_size > 0)
    {
        if(!BootUpdate_CheckTempCRC(actual_size, s_fw_crc32))
            return 0;
    }

    /* 检查魔术字 */
    {
        uint32_t magic = 0;
        BSP_FLASH_Read(FW_TEMP_ADDR, (uint8_t*)&magic, 4);
        if(magic != BOOT_FIRMWARE_MAGIC)
            return 0;
    }

    /* 备份当前 APP */
    if(BSP_FLASH_IsAppValid(APP_START_ADDR))
    {
        BootUpdate_BackupApp(APP_SIZE);
    }

    /* 拷贝暂存区到 APP 区 */
    if(!BootUpdate_CopyTempToApp(actual_size))
        return 0;

    BootFlag_ClearUpdateFlag();
    s_update_started = 0;

    return 1;
}

uint8_t BootUpdate_BackupApp(uint32_t size)
{
    uint32_t offset = 0;
    uint32_t remain = size;
    uint32_t copy_len;

    if(size == 0 || size > APP_BACKUP_SIZE) return 0;
    if(!BSP_FLASH_IsAppValid(APP_START_ADDR)) return 0;

    if(!BSP_FLASH_Erase(APP_BACKUP_ADDR, APP_BACKUP_SIZE))
        return 0;

    while(remain > 0)
    {
        copy_len = (remain > BOOT_UPDATE_CHUNK_SIZE) ? BOOT_UPDATE_CHUNK_SIZE : remain;
        BSP_FLASH_Read(APP_START_ADDR + offset, s_buf, copy_len);
        if(!BSP_FLASH_Write(APP_BACKUP_ADDR + offset, s_buf, copy_len))
            return 0;
        offset += copy_len;
        remain -= copy_len;
    }
    return 1;
}
