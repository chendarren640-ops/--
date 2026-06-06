#ifndef __RTC_DRV_H
#define __RTC_DRV_H
#include "main.h"
void  RTC_Init(void);
void  RTC_SetTime(uint32_t utc_timestamp);
uint32_t RTC_GetTime(void);
void  RTC_SetAlarm(uint32_t seconds_from_now);
#endif
