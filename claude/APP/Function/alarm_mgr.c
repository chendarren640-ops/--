#include "alarm_mgr.h"
#include "rtc_drv.h"
#include "flash_param.h"
#include "frame_builder.h"

static AlarmRecord_t alarm_buf[ALARM_MAX_RECORDS];
static uint8_t alarm_head = 0;   /* 写入位置 */
static uint8_t alarm_count = 0;  /* 当前记录数 */

void Alarm_Init(void) {
    alarm_head = 0;
    alarm_count = 0;
    for (int i = 0; i < ALARM_MAX_RECORDS; i++) {
        alarm_buf[i].utc = 0;
        alarm_buf[i].channel = 0;
        alarm_buf[i].threshold = 0.0f;
        alarm_buf[i].actual = 0.0f;
    }
}

/* 添加一条告警记录 (环形缓冲, 最多10条) */
static void alarm_add(uint8_t channel, float threshold, float actual) {
    alarm_buf[alarm_head].utc       = RTC_GetTime();
    alarm_buf[alarm_head].channel   = channel;
    alarm_buf[alarm_head].threshold = threshold;
    alarm_buf[alarm_head].actual    = actual;

    alarm_head = (alarm_head + 1) % ALARM_MAX_RECORDS;
    if (alarm_count < ALARM_MAX_RECORDS) alarm_count++;
}

/* 检查阈值并触发告警 */
void Alarm_Check(float ch0, float ch1) {
    /* CH0 阈值检查 */
    if (g_param.ch0_threshold > 0.0f && ch0 > g_param.ch0_threshold) {
        alarm_add(0, g_param.ch0_threshold, ch0);
        /* 主动上报模式 → 立即输出字符串 */
        if (g_param.alarm_mode == 0x01) {
            Build_AlarmString(RTC_GetTime(), 0, g_param.ch0_threshold, ch0);
        }
    }

    /* CH1 阈值检查 */
    if (g_param.ch1_threshold > 0.0f && ch1 > g_param.ch1_threshold) {
        alarm_add(1, g_param.ch1_threshold, ch1);
        if (g_param.alarm_mode == 0x01) {
            Build_AlarmString(RTC_GetTime(), 1, g_param.ch1_threshold, ch1);
        }
    }
}

/* 查询告警记录 — 字符串输出 (不组帧, I-02) */
void Alarm_Query(void) {
    if (alarm_count == 0) {
        USART0_DBG_SendString("No alarm records\r\n");
        return;
    }

    /* 从最早记录开始输出 (环形缓冲: 最早在 (head - count) 位置) */
    uint8_t start = (alarm_head + ALARM_MAX_RECORDS - alarm_count) % ALARM_MAX_RECORDS;
    for (uint8_t i = 0; i < alarm_count; i++) {
        uint8_t idx = (start + i) % ALARM_MAX_RECORDS;
        Build_AlarmString(alarm_buf[idx].utc,
                          alarm_buf[idx].channel,
                          alarm_buf[idx].threshold,
                          alarm_buf[idx].actual);
    }
}

/* 清除所有告警记录 (I-03) */
void Alarm_Clear(void) {
    alarm_head = 0;
    alarm_count = 0;
}

/* 设置告警模式: 0x01=主动上报, 0x02=仅存储 (I-04) */
void Alarm_SetMode(uint8_t mode) {
    g_param.alarm_mode = mode;
    Param_Save();
}
