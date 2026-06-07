/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：rtc.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2026/01/30     V0.01    original
************************************************************/

/************************* 头文件 *************************/

#include "rtc.h"

/************************ 全局变量定义 ************************/


/************************************************************
 * Function :       RTC_Init
 * Comment  :       用于初始化RTC端口
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-01-30 V0.1 original
************************************************************/

void my_rtc_init(void)
{
	rtc_parameter_struct   rtc_initpara;
	rtc_alarm_struct  rtc_alarm;

	/* enable PMU clocks */
	rcu_periph_clock_enable(RCU_PMU);
	/* allow access to BKP domain */
	pmu_backup_write_enable();
	/* reset backup domain */
	rcu_bkp_reset_enable();
	rcu_bkp_reset_disable();

	/* enable RCU_LXTAL */
	rcu_osci_on(RCU_LXTAL);
	/* wait till RCU_LXTAL is ready */
	rcu_osci_stab_wait(RCU_LXTAL);
	/* select RCU_LXTAL as RTC clock source */
	rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
	/* enable RTC Clock */
	rcu_periph_clock_enable(RCU_RTC);

	//!分频位1HZ
	rtc_initpara.factor_asyn = 0x63;
	rtc_initpara.factor_syn = 0x13F;
	rtc_initpara.year = 0x16;
	rtc_initpara.day_of_week = RTC_SATURDAY;
	rtc_initpara.month = RTC_APR;
	rtc_initpara.date = 0x30;
	rtc_initpara.display_format = RTC_24HOUR;
	rtc_initpara.am_pm = RTC_AM;

	/* configure current time */
	rtc_initpara.hour = 0x00;
	rtc_initpara.minute = 0x00;
	rtc_initpara.second = 0x00;
	rtc_init(&rtc_initpara);
	rtc_alarm_disable(RTC_ALARM0);
	rtc_alarm.alarm_mask = RTC_ALARM_DATE_MASK | RTC_ALARM_HOUR_MASK | RTC_ALARM_MINUTE_MASK;
	rtc_alarm.weekday_or_date = RTC_ALARM_DATE_SELECTED;
	rtc_alarm.alarm_day = 0x31;
	rtc_alarm.am_pm = RTC_AM;

	/* RTC alarm value */
	rtc_alarm.alarm_hour = 0x00;
	rtc_alarm.alarm_minute = 0x00;
	rtc_alarm.alarm_second = Alarm_Time;	//!Alarm_Time 秒后 触发闹钟
	// rtc_alarm.alarm_second = 0x03;	//!Alarm_Time 秒后 触发闹钟

	/* configure RTC alarm */
	rtc_alarm_config(RTC_ALARM0 , &rtc_alarm);

	rtc_interrupt_enable(RTC_INT_ALARM0);
	rtc_alarm_enable(RTC_ALARM0);

	rtc_flag_clear(RTC_FLAG_ALRM0);


}

/****************************End*****************************/

