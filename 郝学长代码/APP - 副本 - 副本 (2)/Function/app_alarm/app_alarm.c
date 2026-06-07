/*
 * app_alarm.c
 * 告警模块 - 阈值检测、告警记录存储、主动上报
 */

#include "app_alarm.h"
#include "app_param.h"
#include "app_sample.h"
#include "bsp_flash.h"
#include "bsp_rtc.h"
#include "bsp_usart_rs485.h"
#include <string.h>
#include <stdio.h>

/* 告警 Flash 区域地址 (使用 app_alarm.h 中定义的值) */
#define ALARM_FLASH_ADDR    BSP_FLASH_ALARM_ADDR
#define ALARM_FLASH_SIZE    BSP_FLASH_ALARM_SIZE

/* 静态变量 */
static uint8_t s_ch0_alarm_active = 0;
static uint8_t s_ch1_alarm_active = 0;
static uint8_t s_alarm_mode = ALARM_MODE_ACTIVE;  /* 默认主动上报 */

/* 保存一条告警记录到 Flash (循环覆盖) */
static void save_record(const alarm_record_t *record)
{
    uint8_t all_records[ALARM_RECORD_COUNT * sizeof(alarm_record_t)];
    alarm_record_t *recs = (alarm_record_t *)all_records;
    int insert_idx = -1;

    /* 读取所有记录 */
    BSP_FLASH_Read(ALARM_FLASH_ADDR, all_records, sizeof(all_records));

    /* 找到第一个空位 */
    for (int i = 0; i < ALARM_RECORD_COUNT; i++)
    {
        if (recs[i].time_str[0] == 0 || recs[i].time_str[0] == 0xFF)
        {
            insert_idx = i;
            break;
        }
    }

    if (insert_idx == -1)
    {
        /* 满了，覆盖最旧的(第0条)，前移其余记录 */
        for (int i = 1; i < ALARM_RECORD_COUNT; i++)
        {
            memcpy(&recs[i - 1], &recs[i], sizeof(alarm_record_t));
        }
        insert_idx = ALARM_RECORD_COUNT - 1;
    }

    /* 写入新记录 */
    memcpy(&recs[insert_idx], record, sizeof(alarm_record_t));

    /* 擦除告警区并写入全部记录 */
    BSP_FLASH_Erase(ALARM_FLASH_ADDR, ALARM_FLASH_SIZE);
    BSP_FLASH_Write(ALARM_FLASH_ADDR, all_records, sizeof(all_records));
}

/* 发送告警字符串 (直接 ASCII，非帧封装) */
static void send_alarm_string(const char *str)
{
    BSP_RS485_SendData((const uint8_t *)str, strlen(str));
}

/* 触发告警: 存储记录 + 可选主动上报 */
static void trigger_alarm(uint8_t channel, float threshold, float actual_value)
{
    alarm_record_t rec;
    char time_buf[20];

    /* 获取当前时间字符串 */
    RTC_GetDateTimeString(time_buf, sizeof(time_buf));

    memset(&rec, 0, sizeof(rec));
    strncpy((char *)rec.time_str, time_buf, 19);
    rec.channel = channel;
    rec.threshold = threshold;
    rec.actual_value = actual_value;

    /* 保存到 Flash */
    save_record(&rec);

    /* 如果是主动上报模式，发送字符串 */
    if (s_alarm_mode == ALARM_MODE_ACTIVE)
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "%s | CH%d | %.2f | %.2f\r\n",
                 time_buf, channel, threshold, actual_value);
        send_alarm_string(msg);
    }
}

void APP_Alarm_Init(void)
{
    s_alarm_mode = ALARM_MODE_ACTIVE;
    s_ch0_alarm_active = 0;
    s_ch1_alarm_active = 0;
}

void APP_Alarm_Check(const app_sample_t *sample)
{
    if (sample == NULL) return;

    /* 只在告警功能开启时检测 */
    if (g_app_param.alarm_enable == 0) return;

    /* CH0 检测 */
    if (sample->ch0_value > g_app_param.ch0_threshold)
    {
        if (!s_ch0_alarm_active)
        {
            s_ch0_alarm_active = 1;
            trigger_alarm(0, g_app_param.ch0_threshold, sample->ch0_value);
        }
    }
    else
    {
        s_ch0_alarm_active = 0;
    }

    /* CH1 检测 */
    if (sample->ch1_value > g_app_param.ch1_threshold)
    {
        if (!s_ch1_alarm_active)
        {
            s_ch1_alarm_active = 1;
            trigger_alarm(1, g_app_param.ch1_threshold, sample->ch1_value);
        }
    }
    else
    {
        s_ch1_alarm_active = 0;
    }
}

void APP_Alarm_Clear(void)
{
    /* 清除所有告警记录 */
    uint8_t zeros[ALARM_RECORD_COUNT * sizeof(alarm_record_t)];
    memset(zeros, 0, sizeof(zeros));
    BSP_FLASH_Erase(ALARM_FLASH_ADDR, ALARM_FLASH_SIZE);
    BSP_FLASH_Write(ALARM_FLASH_ADDR, zeros, sizeof(zeros));
    s_ch0_alarm_active = 0;
    s_ch1_alarm_active = 0;
}

uint8_t APP_Alarm_GetRecord(uint8_t index, alarm_record_t *record)
{
    if (index >= ALARM_RECORD_COUNT || record == NULL) return 0;

    uint32_t addr = ALARM_FLASH_ADDR + index * sizeof(alarm_record_t);
    BSP_FLASH_Read(addr, (uint8_t *)record, sizeof(alarm_record_t));
    if (record->time_str[0] == 0 || record->time_str[0] == 0xFF)
        return 0;   /* 无效 */
    return 1;
}

uint8_t APP_Alarm_GetCount(void)
{
    uint8_t count = 0;
    for (int i = 0; i < ALARM_RECORD_COUNT; i++)
    {
        alarm_record_t rec;
        if (APP_Alarm_GetRecord(i, &rec))
            count++;
    }
    return count;
}

/*
 * 获取所有告警记录的字符串表示 (按时间倒序)
 * 用于协议层查询告警记录命令
 * 返回有效记录数
 */
uint8_t APP_Get_Alarm_Records(char *buf, uint16_t buf_size)
{
    uint8_t count = APP_Alarm_GetCount();
    uint16_t pos = 0;

    if (count == 0 || buf == NULL || buf_size == 0)
    {
        if (buf != NULL && buf_size > 0)
            buf[0] = '\0';
        return 0;
    }

    /* 按倒序输出 (最新的在前) */
    for (int i = count - 1; i >= 0 && pos < buf_size - 1; i--)
    {
        alarm_record_t rec;
        if (APP_Alarm_GetRecord(i, &rec))
        {
            int n = snprintf(buf + pos, buf_size - pos,
                             "%s | CH%d | %.2f | %.2f\r\n",
                             (char *)rec.time_str, rec.channel,
                             rec.threshold, rec.actual_value);
            if (n > 0) pos += n;
        }
    }

    return count;
}

void APP_Alarm_SetMode(uint8_t mode)
{
    if (mode == ALARM_MODE_ACTIVE || mode == ALARM_MODE_PASSIVE)
        s_alarm_mode = mode;
}

uint8_t APP_Alarm_GetMode(void)
{
    return s_alarm_mode;
}
