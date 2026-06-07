/*
 * app_alarm.h
 * 告警模块头文件
 */

#ifndef __APP_ALARM_H__
#define __APP_ALARM_H__

#include <stdint.h>
#include "app_sample.h"

/* 告警模式 (与 protocol_cmd.h 保持一致) */
#ifndef ALARM_MODE_ACTIVE
#define ALARM_MODE_ACTIVE   0x01   // 主动上报
#endif
#ifndef ALARM_MODE_PASSIVE
#define ALARM_MODE_PASSIVE  0x02   // 仅存储
#endif

/* 告警记录结构体 (32字节) */
typedef struct {
    uint8_t  time_str[20];         // "2026-01-01 12:00:00\0"
    uint8_t  channel;              // 0=CH0, 1=CH1
    uint8_t  reserved[3];          // 保留
    float    threshold;            // 阈值
    float    actual_value;         // 实际值
} alarm_record_t;

/* 告警记录数量 (最多10条，循环覆盖) */
#define ALARM_RECORD_COUNT  10

/* 告警 Flash 地址 (在固件区域内，升级不会擦除) */
#ifndef BSP_FLASH_ALARM_ADDR
#define BSP_FLASH_ALARM_ADDR   0x08071000U
#define BSP_FLASH_ALARM_SIZE   0x00001000U   /* 4KB */
#endif

void APP_Alarm_Init(void);
void APP_Alarm_Check(const app_sample_t *sample);
void APP_Alarm_Clear(void);
uint8_t APP_Alarm_GetRecord(uint8_t index, alarm_record_t *record);
uint8_t APP_Alarm_GetCount(void);
uint8_t APP_Get_Alarm_Records(char *buf, uint16_t buf_size);
void APP_Alarm_SetMode(uint8_t mode);
uint8_t APP_Alarm_GetMode(void);

#endif /* __APP_ALARM_H__ */
