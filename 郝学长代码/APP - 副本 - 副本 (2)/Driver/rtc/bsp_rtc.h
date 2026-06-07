/**
 * @file    bsp_rtc.h
 * @brief   RTC驱动头文件 - 支持UTC时间戳和闹钟功能
 */
#ifndef __BSP_RTC_H__
#define __BSP_RTC_H__

#include "gd32f4xx.h"

/* 将日期时间转换为Unix时间戳（基准1970-01-01 00:00:00 UTC） */
uint32_t bsp_rtc_date_to_unix(uint16_t year, uint8_t month, uint8_t day,
                              uint8_t hour, uint8_t minute, uint8_t second);

/* Unix时间戳转换为日期时间 */
void bsp_rtc_unix_to_date(uint32_t timestamp,
                          uint16_t *year, uint8_t *month, uint8_t *day,
                          uint8_t *hour, uint8_t *minute, uint8_t *second);

/* RTC初始化（选择时钟源，默认初始时间为 2000-01-01 00:00:00 UTC） */
void bsp_rtc_init(void);

/* 设置RTC时间为指定的Unix时间戳 */
void bsp_rtc_set_unix_timestamp(uint32_t timestamp);

/* 获取当前RTC时间（返回Unix时间戳） */
uint32_t bsp_rtc_get_unix_timestamp(void);

/* 设置RTC闹钟（相对时间，seconds 秒后触发） */
void bsp_rtc_set_relative_alarm(uint32_t seconds);

/* 设置RTC闹钟为指定的Unix时间戳 */
void bsp_rtc_set_absolute_alarm(uint32_t timestamp);

/* 禁止RTC闹钟 */
void bsp_rtc_disable_alarm(void);

/* 检查是否有闹钟触发（触发返回1，自动清除标志） */
uint8_t bsp_rtc_check_alarm_flag(void);

/* 获取当前日期时间字符串 "YYYY-MM-DD HH:MM:SS" */
void RTC_GetDateTimeString(char *buf, uint8_t len);

/* RTC闹钟中断处理函数 */
void RTC_Alarm_IRQHandler(void);

/* RTC唤醒中断处理函数 */
void RTC_WKUP_IRQHandler(void);

#endif /* __BSP_RTC_H__ */


