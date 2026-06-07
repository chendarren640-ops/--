#ifndef __ALARM_MGR_H
#define __ALARM_MGR_H
#include "main.h"

#define ALARM_MAX_RECORDS  10

/* Flash 告警存储区 (独立页 0x08071000, 不与参数区冲突) */
#define ALARM_FLASH_ADDR   0x08071000

typedef struct {
    uint32_t utc;
    uint8_t  channel;      /* 0=CH0, 1=CH1 */
    float    threshold;
    float    actual;
} AlarmRecord_t;

void Alarm_Init(void);
void Alarm_Check(float ch0, float ch1);
void Alarm_Query(void);             /* 字符串输出最近10条 */
void Alarm_Clear(void);
void Alarm_SetMode(uint8_t mode);   /* 0x01=主动上报, 0x02=仅存储 */
void Alarm_SaveToFlash(void);       /* 持久化告警到Flash */
#endif
