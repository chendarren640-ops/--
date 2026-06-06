#include "rtc_drv.h"
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
    rtc_lwoff_wait();
    rtc_prescaler_set(0x7F, 0xFF);
    rtc_lwoff_wait();
}
void RTC_SetTime(uint32_t utc) {
    rtc_lwoff_wait();
    rtc_counter_set(utc);
    rtc_lwoff_wait();
}
uint32_t RTC_GetTime(void) {
    return rtc_counter_get();
}
void RTC_SetAlarm(uint32_t seconds_from_now) {
    rtc_alarm_disable();
    rtc_lwoff_wait();
    rtc_alarm_config(RTC_GetTime() + seconds_from_now, RTC_ALARM_TIME_MODE);
    rtc_alarm_enable();
    exti_flag_clear(EXTI_17);
    nvic_irq_enable(RTC_Alarm_IRQn, 0, 0);
    rtc_interrupt_enable(RTC_INT_ALARM);
    rtc_lwoff_wait();
}
