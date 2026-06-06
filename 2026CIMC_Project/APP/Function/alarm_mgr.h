#ifndef __ALARM_MGR_H
#define __ALARM_MGR_H
#include "main.h"
void Alarm_Init(void);
void Alarm_Check(float ch0, float ch1);
void Alarm_Query(void);
void Alarm_Clear(void);
void Alarm_SetMode(uint8_t mode);
#endif
