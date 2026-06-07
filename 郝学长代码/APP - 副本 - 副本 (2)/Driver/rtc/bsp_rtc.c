#include "bsp_rtc.h"
#include <string.h>
#include <stdio.h>

/* 备份寄存器标志值 */
#define BKP_VALUE           0xA5A5

/* 备份寄存器访问宏 */
#define BKP_REG(index)      (*(__IO uint16_t *)(0x40002800 + (index) * 2))

/* 预分频值：使用外部32.768kHz晶振产生1Hz时钟 */
#define PRESCALER_A         127
#define PRESCALER_S         255

/* BCD码与十进制转换宏 */
#define BCD_TO_DEC(bcd)     ( (((bcd) >> 4) * 10) + ((bcd) & 0x0F) )
#define DEC_TO_BCD(dec)     ( (((dec) / 10) << 4) | ((dec) % 10) )

/* RTC唤醒定时器标志定义（兼容旧版本） */
#define RTC_FLAG_WUTF  RTC_FLAG_WT

/* 闹钟触发标志（由中断服务程序设置） */
static volatile uint8_t rtc_alarm_pending = 0;

/* 内部使用的日期时间结构体 */
typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
} rtc_date_time_t;

/* 判断是否为闰年 */
static int8_t is_leap_year(uint16_t year)
{
    if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))
        return 1;
    return 0;
}

/* 将日期时间转换为Unix时间戳 */
static uint32_t date_to_unix(const rtc_date_time_t *dt)
{
    uint32_t days = 0;
    uint16_t y;
    uint8_t m;
    static const uint8_t days_in_months[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

    for (y = 1970; y < dt->year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }
    for (m = 1; m < dt->month; m++) {
        days += days_in_months[m-1];
    }
    if (dt->month > 2 && is_leap_year(dt->year)) days += 1;
    days += (dt->day - 1);

    return days * 86400u + dt->hour * 3600u + dt->minute * 60u + dt->second;
}

/* 将Unix时间戳转换为日期时间 */
static void unix_to_date(uint32_t ts, rtc_date_time_t *dt)
{
    uint32_t days = ts / 86400u;
    uint32_t rem = ts % 86400u;
    uint16_t y;
    uint8_t m, dim;
    static const uint8_t days_in_months[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

    dt->hour   = (uint8_t)(rem / 3600u);
    dt->minute = (uint8_t)((rem % 3600u) / 60u);
    dt->second = (uint8_t)(rem % 60u);

    y = 1970;
    while (1) {
        uint16_t d = is_leap_year(y) ? 366 : 365;
        if (days >= d) { days -= d; y++; }
        else break;
    }
    dt->year = y;

    m = 1;
    while (m <= 12) {
        dim = days_in_months[m-1];
        if (m == 2 && is_leap_year(y)) dim = 29;
        if (days >= dim) { days -= dim; m++; }
        else break;
    }
    dt->month = m;
    dt->day   = (uint8_t)(days + 1);
}

/* ---------- 对外接口函数 ---------- */

/* RTC初始化函数：配置时钟源、预分频器，并检查是否首次上电 */
void bsp_rtc_init(void)
{
    rtc_parameter_struct rtc_initpara;

    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    rcu_osci_on(RCU_LXTAL);
    rcu_osci_stab_wait(RCU_LXTAL);
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);

    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();

    /* 检查备份寄存器标志，判断是否需要初始化RTC */
    if (BKP_REG(1) != BKP_VALUE) {
        memset(&rtc_initpara, 0, sizeof(rtc_initpara));

        rtc_initpara.year            = DEC_TO_BCD(0);
        rtc_initpara.month           = DEC_TO_BCD(1);
        rtc_initpara.date            = DEC_TO_BCD(1);
        rtc_initpara.day_of_week     = RTC_SATURDAY;
        rtc_initpara.hour            = DEC_TO_BCD(0);
        rtc_initpara.minute          = DEC_TO_BCD(0);
        rtc_initpara.second          = DEC_TO_BCD(0);
        rtc_initpara.factor_asyn     = PRESCALER_A;
        rtc_initpara.factor_syn      = PRESCALER_S;
        rtc_initpara.display_format  = RTC_24HOUR;
        rtc_initpara.am_pm           = RTC_AM;

        if (rtc_init(&rtc_initpara) == ERROR) {
            while(1);
        }

        BKP_REG(1) = BKP_VALUE;          /* 设置已初始化标志 */
    } else {
        rtc_register_sync_wait();
    }
}

/* 设置RTC当前时间（通过Unix时间戳） */
void bsp_rtc_set_unix_timestamp(uint32_t timestamp)
{
    rtc_date_time_t dt;
    rtc_parameter_struct rtc_time;

    unix_to_date(timestamp, &dt);

    memset(&rtc_time, 0, sizeof(rtc_time));

    rtc_time.year        = DEC_TO_BCD(dt.year % 100);
    rtc_time.month       = DEC_TO_BCD(dt.month);
    rtc_time.date        = DEC_TO_BCD(dt.day);
    rtc_time.day_of_week = RTC_SATURDAY;
    rtc_time.hour        = DEC_TO_BCD(dt.hour);
    rtc_time.minute      = DEC_TO_BCD(dt.minute);
    rtc_time.second      = DEC_TO_BCD(dt.second);
    rtc_time.display_format = RTC_24HOUR;
    rtc_time.am_pm       = RTC_AM;
    rtc_time.factor_asyn = PRESCALER_A;
    rtc_time.factor_syn  = PRESCALER_S;

    rtc_init(&rtc_time);
}

/* 获取当前Unix时间戳 */
uint32_t bsp_rtc_get_unix_timestamp(void)
{
    rtc_parameter_struct rtc_time;
    rtc_date_time_t dt;

    rtc_current_time_get(&rtc_time);

    dt.year  = 2000 + (uint16_t)BCD_TO_DEC(rtc_time.year);
    dt.month = BCD_TO_DEC(rtc_time.month);
    dt.day   = BCD_TO_DEC(rtc_time.date);
    dt.hour  = BCD_TO_DEC(rtc_time.hour);
    dt.minute= BCD_TO_DEC(rtc_time.minute);
    dt.second= BCD_TO_DEC(rtc_time.second);

    return date_to_unix(&dt);
}

/* 获取格式化的日期时间字符串：YYYY-MM-DD HH:MM:SS */
void RTC_GetDateTimeString(char *buf, uint8_t len)
{
    rtc_parameter_struct rtc_time;
    uint16_t year;
    uint8_t month, day, hour, minute, second;

    rtc_current_time_get(&rtc_time);

    year   = 2000 + (uint16_t)BCD_TO_DEC(rtc_time.year);
    month  = BCD_TO_DEC(rtc_time.month);
    day    = BCD_TO_DEC(rtc_time.date);
    hour   = BCD_TO_DEC(rtc_time.hour);
    minute = BCD_TO_DEC(rtc_time.minute);
    second = BCD_TO_DEC(rtc_time.second);

    snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d",
             year, month, day, hour, minute, second);
}

/* 设置相对闹钟（当前时间 + seconds 秒后触发） */
void bsp_rtc_set_relative_alarm(uint32_t seconds)
{
    uint32_t current = bsp_rtc_get_unix_timestamp();
    bsp_rtc_set_absolute_alarm(current + seconds);
}

/* 设置绝对闹钟（指定Unix时间戳触发） */
void bsp_rtc_set_absolute_alarm(uint32_t timestamp)
{
    rtc_date_time_t dt;
    rtc_alarm_struct alarm;

    unix_to_date(timestamp, &dt);

    memset(&alarm, 0, sizeof(alarm));

    alarm.alarm_mask    = RTC_ALARM_DATE_MASK;
    alarm.weekday_or_date = RTC_ALARM_DATE_SELECTED;
    alarm.alarm_day     = DEC_TO_BCD(dt.day);
    alarm.alarm_hour    = DEC_TO_BCD(dt.hour);
    alarm.alarm_minute  = DEC_TO_BCD(dt.minute);
    alarm.alarm_second  = DEC_TO_BCD(dt.second);
    alarm.am_pm         = RTC_AM;

    rtc_alarm_disable(RTC_ALARM0);
    rtc_alarm_config(RTC_ALARM0, &alarm);

    rtc_flag_clear(RTC_FLAG_ALRM0);
    rtc_interrupt_enable(RTC_INT_ALARM0);
    rtc_alarm_enable(RTC_ALARM0);

    nvic_irq_enable(RTC_Alarm_IRQn, 1, 0);
}

/* 关闭闹钟 */
void bsp_rtc_disable_alarm(void)
{
    rtc_alarm_disable(RTC_ALARM0);
    rtc_interrupt_disable(RTC_INT_ALARM0);
    rtc_alarm_pending = 0;
}

/* 检查闹钟是否触发，若触发则清除标志并返回1，否则返回0 */
uint8_t bsp_rtc_check_alarm_flag(void)
{
    uint8_t flag = rtc_alarm_pending;
    if (flag) {
        rtc_alarm_pending = 0;
        rtc_flag_clear(RTC_FLAG_ALRM0);
    }
    return flag;
}

/* RTC闹钟中断服务函数 */
void RTC_Alarm_IRQHandler(void)
{
    if (rtc_flag_get(RTC_FLAG_ALRM0) != RESET) {
        rtc_flag_clear(RTC_FLAG_ALRM0);
        rtc_alarm_pending = 1;         /* 置位闹钟触发标志 */
    }
}

/* RTC唤醒中断服务函数（本示例暂未使用唤醒定时器） */
void RTC_WKUP_IRQHandler(void)
{
    if (rtc_flag_get(RTC_FLAG_WUTF) != RESET)
    {
        rtc_flag_clear(RTC_FLAG_WUTF);
    }
}


