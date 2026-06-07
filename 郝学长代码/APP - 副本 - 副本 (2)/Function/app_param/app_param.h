/*
 * app_param.h
 * 参数管理模块 - 持久化存储到内部 Flash
 */

#ifndef __APP_PARAM_H__
#define __APP_PARAM_H__

#include "gd32f4xx.h"
#include <stdint.h>
#include "bsp_flash.h"

#define APP_PARAM_MAGIC         0x20264666UL

/* 版本号 2.0.1.0 */
#define APP_VERSION_MAJOR       2
#define APP_VERSION_MINOR       0
#define APP_VERSION_PATCH       1
#define APP_VERSION_BUILD       0
#define APP_VERSION_NUM         ((APP_VERSION_MAJOR << 24) | (APP_VERSION_MINOR << 16) | \
                                 (APP_VERSION_PATCH << 8) | APP_VERSION_BUILD)

/* 默认参数值 */
#define APP_DEFAULT_DEVICE_ID       0x0001
#define APP_DEFAULT_BAUDRATE        0x13        /* 映射值: 0x13=19200 */

#define APP_DEFAULT_CH0_RATIO       1.0f
#define APP_DEFAULT_CH1_RATIO       1.0f

#define APP_DEFAULT_CH0_THRESHOLD   3.0f
#define APP_DEFAULT_CH1_THRESHOLD   50.0f

#define APP_DEFAULT_REPORT_INTERVAL 0x01   /* 1=1s, 2=3s, 3=5s (协议映射码) */

/* ADC 参考值 (供 app_sample 使用) */
#define APP_ADC_REF_VOLTAGE     3.3f
#define APP_ADC_MAX_VALUE       4095.0f

/* 参数结构体 - 与协议层共用 */
typedef struct
{
    uint32_t magic;                         /* 魔术字 */
    uint32_t version;                       /* 版本号 */

    uint16_t device_id;                     /* 设备地址 */
    uint8_t  baudrate;                      /* 波特率映射值: 0x11=4800, 0x12=9600, 0x13=19200, 0x14=115200 */
    uint8_t  autoreport_interval;           /* 上报间隔映射码: 1=1s, 2=3s, 3=5s */

    uint8_t  autoreport_enable;             /* 1:开启自动上报 */
    uint8_t  alarm_enable;                  /* 0:关闭, 1:主动上报, 2:仅存储 */
    uint8_t  reserved1[2];                  /* 保留 */

    float ch0_ratio;                        /* CH0 变比系数 */
    float ch1_ratio;                        /* CH1 变比系数 */

    float ch0_threshold;                    /* CH0 报警阈值 */
    float ch1_threshold;                    /* CH1 报警阈值 */

    uint32_t crc32;                         /* CRC32 校验（除本字段外） */
} app_param_t;

extern app_param_t g_app_param;

/* 参数操作函数 */
void APP_Param_Init(void);
void APP_Param_LoadDefault(void);
uint8_t APP_Param_Load(void);
uint8_t APP_Param_Save(void);

/* 波特率映射转换 */
uint32_t APP_Param_GetBaudrateValue(uint8_t map);

/* 上报间隔映射转换 (协议码 -> 毫秒) */
uint32_t APP_Param_GetIntervalMs(uint8_t map);

#endif /* __APP_PARAM_H__ */
