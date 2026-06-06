#include "rtc_drv.h"
#include <string.h>

void RTC_Init(void) {
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
    rcu_bkp_reset_enable();
    rcu_bkp_reset_disable();
    rcu_osci_on(RCU_LXTAL);
    while(SUCCESS != rcu_osci_stab_wait(RCU_LXTAL));
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();

    rtc_parameter_struct rtc_initpara;
    memset(&rtc_initpara, 0, sizeof(rtc_initpara));
    rtc_initpara.factor_asyn = 0x7F;
    rtc_initpara.factor_syn   = 0xFF;
    rtc_initpara.display_format = RTC_24HOUR;
    rtc_init(&rtc_initpara);
}

void RTC_SetTime(uint32_t utc) {
    rtc_parameter_struct rtc_initpara;
    memset(&rtc_initpara, 0, sizeof(rtc_initpara));
    rtc_initpara.factor_asyn = 0x7F;
    rtc_initpara.factor_syn   = 0xFF;
    rtc_initpara.display_format = RTC_24HOUR;
    rtc_init(&rtc_initpara);
}

uint32_t RTC_GetTime(void) {
    rtc_parameter_struct rtc_time;
    rtc_current_time_get(&rtc_time);
    return 0;
}

void RTC_SetAlarm(uint32_t seconds_from_now) {
    rtc_alarm_disable(RTC_ALARM0);
    rtc_register_sync_wait();
    rtc_alarm_struct alarm;
    memset(&alarm, 0, sizeof(alarm));
    alarm.alarm_mask = RTC_ALARM_HOUR_MASK | RTC_ALARM_MINUTE_MASK;
    alarm.weekday_or_date = RTC_ALARM_DATE_SELECTED;
    alarm.alarm_day   = 1;
    alarm.alarm_hour  = 0;
    alarm.alarm_minute= 0;
    alarm.alarm_second= 10;
    alarm.am_pm = RTC_AM;
    rtc_alarm_config(RTC_ALARM0, &alarm);
    rtc_alarm_enable(RTC_ALARM0);
    exti_flag_clear(EXTI_17);
    nvic_irq_enable(RTC_Alarm_IRQn, 0, 0);
    rtc_interrupt_enable(RTC_INT_ALARM0);
    rtc_register_sync_wait();
}
