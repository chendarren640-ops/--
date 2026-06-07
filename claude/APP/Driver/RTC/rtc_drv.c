/**
 * 软件 RTC (UTC时间戳 + 毫秒计数器)
 */
#include "rtc_drv.h"
#include "systick.h"

static uint32_t s_time_base_utc = 0U;
static uint32_t s_time_base_ms  = 0U;

void RTC_Init(void) {
    /* 默认基准: 2026-01-01 00:00:00 UTC */
    s_time_base_utc = 1767225600UL;
    s_time_base_ms  = millis();
}

void RTC_SetTime(uint32_t utc) {
    s_time_base_utc = utc;
    s_time_base_ms  = millis();
}

uint32_t RTC_GetTime(void) {
    uint32_t elapsed_ms = millis() - s_time_base_ms;
    return s_time_base_utc + (elapsed_ms / 1000U);
}

void RTC_WakeupTimer_Start(uint16_t seconds) {
    if (seconds == 0) seconds = 1;
    rcu_periph_clock_enable(RCU_PMU);
    rtc_flag_clear(RTC_FLAG_WT);
    rtc_wakeup_disable();
    rtc_wakeup_clock_set(WAKEUP_CKSPRE);
    rtc_wakeup_timer_set((uint16_t)(seconds - 1U));
    rtc_interrupt_enable(RTC_INT_WAKEUP);
    exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    nvic_irq_enable(RTC_WKUP_IRQn, 1U, 0U);
    rtc_wakeup_enable();
}

void RTC_SetAlarm(uint32_t seconds_from_now) {
    /* RTC 唤醒定时器 */
    RTC_WakeupTimer_Start((uint16_t)seconds_from_now);
}
