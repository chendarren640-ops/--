#ifndef __ALARM_MGR_H
#define __ALARM_MGR_H
#include "main.h"

#define ALARM_MAX_RECORDS  10

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
#endif
