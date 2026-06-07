/**
 * 2026 CIMC APP — RTC 驱动 (参考: bsp_rtc_init + bsp_rtc_setup)
 *
 * RTC_SetTime(utc): 将 UTC 时间戳转为 RTC 寄存器字段并写入
 * RTC_GetTime():    从 RTC 寄存器读取字段, 转回 UTC 时间戳
 */
#include "rtc_drv.h"
#include <string.h>
#include <time.h>

/* 判断闰年 */
static uint8_t is_leap_year(uint16_t year) {
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

/* 每月天数 (非闰年) */
static const uint8_t days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};

/* UTC 时间戳 → RTC 字段 (参考 Unix time conversion) */
static void utc_to_rtc_fields(uint32_t utc, rtc_parameter_struct *rtc)
{
    uint32_t days, secs;
    uint16_t year;
    uint8_t  month;

    memset(rtc, 0, sizeof(*rtc));

    secs = utc % 86400UL;
    days = utc / 86400UL;

    rtc->hour   = (uint8_t)((secs / 3600UL) % 24UL);
    rtc->minute = (uint8_t)((secs / 60UL) % 60UL);
    rtc->second = (uint8_t)(secs % 60UL);

    /* 从 1970-01-01 开始推算 */
    year = 1970;
    while (1) {
        uint16_t days_in_year = is_leap_year(year) ? 366U : 365U;
        if (days < days_in_year) break;
        days -= days_in_year;
        year++;
    }

    rtc->year  = (uint8_t)(year - 2000U); /* RTC 存储 0x00=2000 */
    rtc->month = RTC_JAN;
    for (month = 1; month <= 12; month++) {
        uint8_t dim = days_in_month[month - 1];
        if (month == 2 && is_leap_year(year)) dim = 29;
        if (days < dim) break;
        days -= dim;
        rtc->month = (uint8_t)(month);
    }
    rtc->date = (uint8_t)(days + 1U);

    rtc->display_format = RTC_24HOUR;
    rtc->am_pm = RTC_AM;
    rtc->day_of_week = RTC_MONDAY; /* 不重要 */
}

/* RTC 字段 → UTC 时间戳 */
static uint32_t rtc_fields_to_utc(rtc_parameter_struct *rtc)
{
    uint32_t days = 0;
    uint16_t year;
    uint8_t  month;

    /* 从 1970 到 (2000 + rtc->year) 的天数 */
    for (year = 1970; year < (uint16_t)(2000U + rtc->year); year++) {
        days += is_leap_year(year) ? 366UL : 365UL;
    }

    /* 当前年从 1 月到当前月的天数 */
    for (month = 1; month < rtc->month; month++) {
        days += days_in_month[month - 1];
        if (month == 2 && is_leap_year(2000U + rtc->year)) days++;
    }

    /* 加上当月天数 */
    days += (uint32_t)(rtc->date - 1U);

    return (days * 86400UL) + ((uint32_t)rtc->hour * 3600UL)
         + ((uint32_t)rtc->minute * 60UL) + (uint32_t)rtc->second;
}

void RTC_Init(void)
{
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    /* 使能 LXTAL 32.768kHz */
    rcu_osci_on(RCU_LXTAL);
    while (SUCCESS != rcu_osci_stab_wait(RCU_LXTAL));
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();

    /* 仅首次上电初始化 RTC (BKP 标记防重复) */
    if (RTC_BKP0 != 0xA5A5U) {
        rtc_parameter_struct rtc_cfg;
        rtc_cfg.factor_asyn     = 0x7F;
        rtc_cfg.factor_syn      = 0xFF;
        rtc_cfg.display_format  = RTC_24HOUR;
        rtc_cfg.am_pm           = RTC_AM;
        rtc_init(&rtc_cfg);
        RTC_BKP0 = 0xA5A5U;
    }
}

void RTC_SetTime(uint32_t utc)
{
    rtc_parameter_struct rtc_cfg;

    /* 1. 保留 RTC 分频系数 */
    rtc_current_time_get(&rtc_cfg);
    rtc_cfg.factor_asyn = 0x7F;
    rtc_cfg.factor_syn  = 0xFF;

    /* 2. UTC → RTC 字段 */
    utc_to_rtc_fields(utc, &rtc_cfg);

    /* 3. 写入 RTC */
    rtc_init(&rtc_cfg);
}

uint32_t RTC_GetTime(void)
{
    rtc_parameter_struct rtc_cfg;
    rtc_current_time_get(&rtc_cfg);
    return rtc_fields_to_utc(&rtc_cfg);
}

/* RTC 唤醒定时器 (用于深度睡眠唤醒, 参考 bsp_rtc_wakeup_timer_start) */
void RTC_WakeupTimer_Start(uint16_t seconds)
{
    if (seconds == 0) seconds = 1;

    exti_interrupt_flag_clear(EXTI_22);
    rtc_flag_clear(RTC_FLAG_WT);
    rtc_wakeup_disable();
    rtc_wakeup_clock_set(WAKEUP_CKSPRE);  /* 1Hz 唤醒时钟 */
    rtc_wakeup_timer_set((uint16_t)(seconds - 1U));
    rtc_interrupt_enable(RTC_INT_WAKEUP);
    exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    nvic_irq_enable(RTC_WKUP_IRQn, 1U, 0U);
    rtc_wakeup_enable();
}

void RTC_SetAlarm(uint32_t seconds_from_now)
{
    rtc_parameter_struct rtc_cfg;
    rtc_alarm_struct      alarm;

    rtc_alarm_disable(RTC_ALARM0);
    rtc_register_sync_wait();

    rtc_current_time_get(&rtc_cfg);

    /* 在当前时间上加 seconds_from_now */
    uint32_t cur_secs = (uint32_t)rtc_cfg.hour * 3600U
                      + (uint32_t)rtc_cfg.minute * 60U
                      + (uint32_t)rtc_cfg.second + seconds_from_now;

    memset(&alarm, 0, sizeof(alarm));
    alarm.alarm_mask = RTC_ALARM_HOUR_MASK | RTC_ALARM_MINUTE_MASK;
    alarm.weekday_or_date = RTC_ALARM_DATE_SELECTED;
    alarm.alarm_day   = 1;
    alarm.alarm_hour  = 0;
    alarm.alarm_minute= 0;
    alarm.alarm_second= (uint8_t)(cur_secs % 60U);
    alarm.am_pm = RTC_AM;

    rtc_alarm_config(RTC_ALARM0, &alarm);
    rtc_alarm_enable(RTC_ALARM0);
    exti_flag_clear(EXTI_17);
    nvic_irq_enable(RTC_Alarm_IRQn, 0, 0);
    rtc_interrupt_enable(RTC_INT_ALARM0);
    rtc_register_sync_wait();
}
