#include "data_channel.h"
#include "adc_drv.h"
#include "rtc_drv.h"
#include "flash_param.h"
#include "frame_builder.h"
#include "alarm_mgr.h"
#include "../Driver/PT100/pt100_drv.h"
#include "../Driver/OLED/oled_drv.h"

/* 自动上报状态 */
static uint32_t auto_last_tick = 0;
static uint8_t  auto_interval_s = 0;  /* 0=停止, 1/3/5=秒 */

float Channel_ReadCH0(void) {
    return ADC_ReadCH0() * g_param.ch0_ratio;
}

float Channel_ReadCH1(void) {
    return ADC_ReadCH1() * g_param.ch1_ratio;
}

float Channel_ReadCH2(void) {
    /* PT100 通过 GD30AD3340 (I2C PB8/PB9) 读取真实温度 */
    float t = PT100_ReadTemperature();
    if (t <= -273.0f) return 25.0f; /* 读取失败返回默认 */
    return t;
}

void Channel_AutoSample_SetInterval(uint8_t interval) {
    auto_interval_s = interval;  /* 仅设定间隔, 不改变 g_sys_state */
}

void Channel_AutoSample_Start(uint8_t interval) {
    if (interval > 0) auto_interval_s = interval;
    auto_last_tick = g_sys_tick;
    g_sys_state = STATE_AUTO_SAMPLE;
    LED_SAMPLE_ON();                                  /* 采集指示灯常亮 */
    OLED_ShowLine2((uint8_t*)"AutoSample");           /* OLED 状态切换 */
    OLED_Refresh();
}

void Channel_AutoSample_Stop(void) {
    auto_interval_s = 0;
    g_sys_state = STATE_IDLE;
    LED_SAMPLE_OFF();                                 /* 采集指示灯灭 */
    OLED_ShowLine2((uint8_t*)"IDLE");                 /* OLED 恢复 IDLE */
    OLED_Refresh();
}

/* 主循环中调用: 检查是否到达采样间隔, 是则发送自动上报帧 */
void Channel_AutoSample_Process(void) {
    if (g_sys_state != STATE_AUTO_SAMPLE || auto_interval_s == 0)
        return;

    /* SysTick 周期 = 1ms (1000Hz) */
    uint32_t elapsed_ms = g_sys_tick - auto_last_tick;
    uint32_t interval_ms = (uint32_t)auto_interval_s * 1000;

    if (elapsed_ms >= interval_ms) {
        auto_last_tick = g_sys_tick;

        float ch0 = Channel_ReadCH0();
        float ch1 = Channel_ReadCH1();

        /* 发送自动上报帧: UTC(4B) + CH0(IEEE754 4B) + CH1(IEEE754 4B) = 12B */
        Build_AutoSampleFrame(RTC_GetTime(), ch0, ch1);

        /* 检查告警阈值 */
        Alarm_Check(ch0, ch1);
    }
}
