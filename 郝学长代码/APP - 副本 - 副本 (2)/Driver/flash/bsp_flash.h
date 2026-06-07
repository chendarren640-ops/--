#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include "gd32f4xx.h"
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Flash ������Χ��������GD32���ͻ�� */
#ifndef FLASH_BASE_ADDR
#define FLASH_BASE_ADDR         0x08000000U
#endif
#define FLASH_TOTAL_SIZE        0x00100000U   /* 1MB */

/* ������� */
#define BOOT_START_ADDR         0x08000000U
#define BOOT_SIZE               0x00010000U   /* 64KB */

#ifndef PARAM_START_ADDR
#define PARAM_START_ADDR        0x08010000U
#endif
#ifndef PARAM_SIZE
#define PARAM_SIZE              0x00001000U   /* 4KB */
#endif

#define APP_START_ADDR          0x08011000U
#define APP_SIZE                0x00020000U   /* 128KB */

#define APP_BACKUP_ADDR         0x08031000U
#define APP_BACKUP_SIZE         0x00020000U   /* 128KB */

#define FW_TEMP_ADDR            0x08051000U
#define FW_TEMP_SIZE            0x00020000U   /* 128KB */

/* 告警Flash区域 - 由 app_alarm.h 统一定义 */

/* ����ӿ����� */
uint8_t BSP_FLASH_Erase(uint32_t start_addr, uint32_t size);
uint8_t BSP_FLASH_Write(uint32_t addr, const uint8_t *data, uint32_t len);
void    BSP_FLASH_Read(uint32_t addr, uint8_t *data, uint32_t len);
uint32_t BSP_FLASH_ReadWord(uint32_t addr);
uint8_t BSP_FLASH_IsAppValid(uint32_t app_addr);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_FLASH_H */



