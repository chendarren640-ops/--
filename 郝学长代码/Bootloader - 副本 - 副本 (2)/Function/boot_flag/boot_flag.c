#include "boot_flag.h"
#include "protocol_crc.h"
#include <string.h>

void BootFlag_LoadDefault(boot_param_t *param)
{
    if(param == 0)
    {
        return;
    }

    memset(param, 0xFF, sizeof(boot_param_t));

    param->magic = BOOT_PARAM_MAGIC;
    param->version = BOOT_PARAM_VERSION;

    param->enter_boot_flag = BOOT_FLAG_NONE;
    param->update_flag = BOOT_FLAG_NONE;

    param->app_addr = APP_START_ADDR;
    param->app_size = 0;
    param->app_crc32 = 0xFFFFFFFFUL;
    param->app_version = 0;

    param->backup_addr = APP_BACKUP_ADDR;
    param->temp_addr = FW_TEMP_ADDR;

    param->struct_crc32 = Protocol_CRC32((const uint8_t *)param,
                                         (uint32_t)(sizeof(boot_param_t) - 4U));
}

uint8_t BootFlag_Load(boot_param_t *param)
{
    uint32_t crc;

    if(param == 0)
    {
        return 0;
    }

    BSP_FLASH_Read(PARAM_START_ADDR, (uint8_t *)param, sizeof(boot_param_t));

    if(param->magic != BOOT_PARAM_MAGIC)
    {
        return 0;
    }

    crc = Protocol_CRC32((const uint8_t *)param,
                         (uint32_t)(sizeof(boot_param_t) - 4U));

    if(crc != param->struct_crc32)
    {
        return 0;
    }

    return 1;
}

uint8_t BootFlag_Save(const boot_param_t *param)
{
    boot_param_t temp;

    if(param == 0)
    {
        return 0;
    }

    memcpy(&temp, param, sizeof(boot_param_t));

    temp.struct_crc32 = Protocol_CRC32((const uint8_t *)&temp,
                                       (uint32_t)(sizeof(boot_param_t) - 4U));

    if(!BSP_FLASH_Erase(PARAM_START_ADDR, PARAM_SIZE))
    {
        return 0;
    }

    if(!BSP_FLASH_Write(PARAM_START_ADDR, (const uint8_t *)&temp, sizeof(boot_param_t)))
    {
        return 0;
    }

    return 1;
}

uint8_t BootFlag_SetEnterBootFlag(void)
{
    boot_param_t param;

    if(!BootFlag_Load(&param))
    {
        BootFlag_LoadDefault(&param);
    }

    param.enter_boot_flag = BOOT_ENTER_FLAG;

    return BootFlag_Save(&param);
}

uint8_t BootFlag_ClearEnterBootFlag(void)
{
    boot_param_t param;

    if(!BootFlag_Load(&param))
    {
        BootFlag_LoadDefault(&param);
    }

    param.enter_boot_flag = BOOT_FLAG_NONE;

    return BootFlag_Save(&param);
}

uint8_t BootFlag_SetUpdateInfo(uint32_t size, uint32_t crc32, uint32_t version)
{
    boot_param_t param;

    if(!BootFlag_Load(&param))
    {
        BootFlag_LoadDefault(&param);
    }

    param.update_flag = BOOT_UPDATE_FLAG;
    param.app_size = size;
    param.app_crc32 = crc32;
    param.app_version = version;
    param.app_addr = APP_START_ADDR;
    param.backup_addr = APP_BACKUP_ADDR;
    param.temp_addr = FW_TEMP_ADDR;

    return BootFlag_Save(&param);
}

uint8_t BootFlag_ClearUpdateFlag(void)
{
    boot_param_t param;

    if(!BootFlag_Load(&param))
    {
        BootFlag_LoadDefault(&param);
    }

    param.update_flag = BOOT_FLAG_NONE;
    param.enter_boot_flag = BOOT_FLAG_NONE;

    return BootFlag_Save(&param);
}

uint8_t BootFlag_NeedEnterBoot(void)
{
    boot_param_t param;

    if(!BootFlag_Load(&param))
    {
        return 0;
    }

    return param.enter_boot_flag == BOOT_ENTER_FLAG;
}

uint8_t BootFlag_NeedUpdate(void)
{
    boot_param_t param;

    if(!BootFlag_Load(&param))
    {
        return 0;
    }

    return param.update_flag == BOOT_UPDATE_FLAG;
}

