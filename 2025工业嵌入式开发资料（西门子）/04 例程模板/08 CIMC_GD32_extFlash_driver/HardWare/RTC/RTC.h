#ifndef __RTC_H
#define __RTC_H
#include "HeaderFiles.h"

void RTC_Init(void);
void rtc_setup(void);
void rtc_show_time(void);
void rtc_show_alarm(void);
uint8_t usart_input_threshold(uint32_t value);
void rtc_pre_config(void);

#endif
