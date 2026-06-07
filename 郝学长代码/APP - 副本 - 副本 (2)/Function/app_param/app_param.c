/*
 * app_param.c
 * 参数管理模块 - Flash 持久化
 */

#include "app_param.h"
#include "bsp_flash.h"
#include <string.h>

app_param_t g_app_param;

/* 简单 CRC 校验 */
static uint32_t app_param_simple_crc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0x12345678;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= (uint32_t)data[i];
        crc = (crc << 5) | (crc >> 27);
        crc += 0x9E3779B9;
    }
    return crc;
}

/* 加载默认参数 */
void APP_Param_LoadDefault(void)
{
    memset(&g_app_param, 0, sizeof(app_param_t));

    g_app_param.magic               = APP_PARAM_MAGIC;
    g_app_param.version             = APP_VERSION_NUM;
    g_app_param.device_id           = APP_DEFAULT_DEVICE_ID;        // 0x0001
    g_app_param.baudrate            = APP_DEFAULT_BAUDRATE;         // 0x13 (19200)
    g_app_param.autoreport_interval = APP_DEFAULT_REPORT_INTERVAL;  // 0x01
    g_app_param.autoreport_enable   = 0;
    g_app_param.alarm_enable        = 0;
    g_app_param.ch0_ratio           = APP_DEFAULT_CH0_RATIO;       // 1.0f
    g_app_param.ch1_ratio           = APP_DEFAULT_CH1_RATIO;       // 1.0f
    g_app_param.ch0_threshold       = APP_DEFAULT_CH0_THRESHOLD;   // 3.0f
    g_app_param.ch1_threshold       = APP_DEFAULT_CH1_THRESHOLD;   // 50.0f

    g_app_param.crc32 = app_param_simple_crc((const uint8_t *)&g_app_param,
                        sizeof(app_param_t) - sizeof(uint32_t));
}

/* 从 Flash 加载参数 */
uint8_t APP_Param_Load(void)
{
    app_param_t temp;
    uint32_t calc_crc;

    /* 从 Flash 读取整个参数结构体 */
    BSP_FLASH_Read(PARAM_START_ADDR, (uint8_t *)&temp, sizeof(app_param_t));

    /* 校验 CRC */
    calc_crc = app_param_simple_crc((const uint8_t *)&temp,
                                    sizeof(app_param_t) - sizeof(uint32_t));

    if (calc_crc != temp.crc32)
    {
        APP_Param_LoadDefault();
        APP_Param_Save();
        return 0;   // 使用默认参数
    }

    /* 参数合法性检查 */
    if (temp.device_id == 0x0000 || temp.device_id == 0xFFFF)
    {
        APP_Param_LoadDefault();
        APP_Param_Save();
        return 0;
    }

    /* 波特率映射值合法范围: 0x11~0x14 */
    if (temp.baudrate < 0x11 || temp.baudrate > 0x14)
    {
        APP_Param_LoadDefault();
        APP_Param_Save();
        return 0;
    }

    memcpy(&g_app_param, &temp, sizeof(app_param_t));
    return 1;   // 成功加载
}

/* 保存参数到 Flash（带擦写验证） */
uint8_t APP_Param_Save(void)
{
    app_param_t verify;
    uint32_t calc_crc;
    uint8_t retry;

    /* 重新计算 CRC */
    g_app_param.crc32 = app_param_simple_crc((const uint8_t *)&g_app_param,
                        sizeof(app_param_t) - sizeof(uint32_t));

    for (retry = 0; retry < 2; retry++)
    {
        /* 擦除参数区 */
        if (!BSP_FLASH_Erase(PARAM_START_ADDR, PARAM_SIZE))
        {
            continue;  /* 擦除失败，重试 */
        }

        /* 写入 */
        if (!BSP_FLASH_Write(PARAM_START_ADDR, (const uint8_t *)&g_app_param, sizeof(app_param_t)))
        {
            continue;  /* 写入失败，重试 */
        }

        /* 读回验证 */
        BSP_FLASH_Read(PARAM_START_ADDR, (uint8_t *)&verify, sizeof(app_param_t));

        /* 验证魔术字 */
        if (verify.magic != g_app_param.magic)
        {
            continue;
        }

        /* 验证 CRC */
        calc_crc = app_param_simple_crc((const uint8_t *)&verify,
                                       sizeof(app_param_t) - sizeof(uint32_t));
        if (calc_crc != verify.crc32)
        {
            continue;
        }

        /* 验证关键字段 */
        if (verify.device_id != g_app_param.device_id ||
            verify.baudrate != g_app_param.baudrate)
        {
            continue;
        }

        return 1;  /* 保存并验证成功 */
    }

    return 0;  /* 保存失败 */
}

/* 参数模块初始化 */
void APP_Param_Init(void)
{
    APP_Param_Load();
}

/*
 * 波特率映射转换: 协议码 -> 实际波特率值
 * 0x11=4800, 0x12=9600, 0x13=19200, 0x14=115200
 */
uint32_t APP_Param_GetBaudrateValue(uint8_t map)
{
    switch (map)
    {
        case 0x11: return 4800;
        case 0x12: return 9600;
        case 0x13: return 19200;
        case 0x14: return 115200;
        default:   return 19200;  // 默认 19200
    }
}

/*
 * 上报间隔映射转换: 协议码 -> 毫秒
 * 0x01=1000ms, 0x02=3000ms, 0x03=5000ms
 */
uint32_t APP_Param_GetIntervalMs(uint8_t map)
{
    switch (map)
    {
        case 0x01: return 1000;
        case 0x02: return 3000;
        case 0x03: return 5000;
        default:   return 1000;
    }
}
