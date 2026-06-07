#ifndef __BOOT_FLAG_H
#define __BOOT_FLAG_H

#include <stdint.h>
#include "bsp_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_PARAM_MAGIC             0x20264666UL
#define BOOT_PARAM_VERSION           0x00000001UL

#define BOOT_ENTER_FLAG              0xA55A5AA5UL
#define BOOT_UPDATE_FLAG             0x5AA55AA5UL
#define BOOT_FLAG_NONE               0xFFFFFFFFUL

typedef struct
{
    uint32_t magic;
    uint32_t version;

    uint32_t enter_boot_flag;
    uint32_t update_flag;

    uint32_t app_addr;
    uint32_t app_size;
    uint32_t app_crc32;
    uint32_t app_version;

    uint32_t backup_addr;
    uint32_t temp_addr;

    uint32_t reserved[8];

    uint32_t struct_crc32;
} boot_param_t;

uint8_t BootFlag_Load(boot_param_t *param);
uint8_t BootFlag_Save(const boot_param_t *param);
void BootFlag_LoadDefault(boot_param_t *param);

uint8_t BootFlag_SetEnterBootFlag(void);
uint8_t BootFlag_ClearEnterBootFlag(void);

uint8_t BootFlag_SetUpdateInfo(uint32_t size, uint32_t crc32, uint32_t version);
uint8_t BootFlag_ClearUpdateFlag(void);

uint8_t BootFlag_NeedEnterBoot(void);
uint8_t BootFlag_NeedUpdate(void);

#ifdef __cplusplus
}
#endif

#endif

