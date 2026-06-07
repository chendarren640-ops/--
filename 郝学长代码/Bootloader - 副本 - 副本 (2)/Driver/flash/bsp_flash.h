/*
 * bsp_flash.h
 * GD32F470 Flash 驱动头文件
 * Flash 布局与赛题要求一致
 */

#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include "gd32f4xx.h"
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GD32F470 Flash 扇区布局 (1MB):
 * Sector 0   0x08000000 ~ 0x08003FFF   16KB
 * Sector 1   0x08004000 ~ 0x08007FFF   16KB
 * Sector 2   0x08008000 ~ 0x0800BFFF   16KB
 * Sector 3   0x0800C000 ~ 0x0800FFFF   16KB
 * Sector 4   0x08010000 ~ 0x0801FFFF   64KB
 * Sector 5   0x08020000 ~ 0x0803FFFF   128KB
 * Sector 6   0x08040000 ~ 0x0805FFFF   128KB
 * Sector 7   0x08060000 ~ 0x0807FFFF   128KB
 * Sector 8   0x08080000 ~ 0x0809FFFF   128KB
 * Sector 9   0x080A0000 ~ 0x080BFFFF   128KB
 * Sector 10  0x080C0000 ~ 0x080DFFFF   128KB
 * Sector 11  0x080E0000 ~ 0x080FFFFF   128KB
 */

/* Flash 基本信息 */
#define FLASH_BASE_ADDR             0x08000000UL
#define FLASH_TOTAL_SIZE            0x00100000UL

/*
 * 赛题要求的 Flash 空间划分:
 * Bootloader:  0x08000000 ~ 0x0800FFFF  64K
 * 参数区:      0x08010000 ~ 0x08010FFF  4K
 * App:         0x08011000 ~ 0x08030FFF  128K
 * App 备份:    0x08031000 ~ 0x08050FFF  128K
 * 固件暂存:    0x08051000 ~ 0x08070FFF  128K
 */

/* Bootloader 区 */
#define BOOT_START_ADDR             0x08000000UL
#define BOOT_SIZE                   0x00010000UL   /* 64KB */

/* 参数区 */
#define PARAM_START_ADDR            0x08010000UL
#define PARAM_SIZE                  0x00001000UL   /* 4KB */

/* APP 区 */
#define APP_START_ADDR              0x08011000UL
#define APP_SIZE                    0x00020000UL   /* 128KB */

/* APP 备份区 */
#define APP_BACKUP_ADDR             0x08031000UL
#define APP_BACKUP_SIZE             0x00020000UL   /* 128KB */

/* 固件暂存区 */
#define FW_TEMP_ADDR                0x08051000UL
#define FW_TEMP_SIZE                0x00020000UL   /* 128KB */

/* 魔术字 */
#define BOOT_FIRMWARE_MAGIC         0x5AA5C33CUL

/* Flash API */
uint8_t BSP_FLASH_Erase(uint32_t start_addr, uint32_t size);
uint8_t BSP_FLASH_Write(uint32_t addr, const uint8_t *data, uint32_t len);
void    BSP_FLASH_Read(uint32_t addr, uint8_t *data, uint32_t len);
uint32_t BSP_FLASH_ReadWord(uint32_t addr);
uint8_t BSP_FLASH_IsAppValid(uint32_t app_addr);

#ifdef __cplusplus
}
#endif

#endif
