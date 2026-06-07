#include "data_channel.h"
#include "adc_drv.h"
#include "rtc_drv.h"
#include "flash_param.h"
#include "frame_builder.h"
#include "alarm_mgr.h"

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
    /* PT100 温度传感器 — 通过外部 ADC 读取, 默认返回 25°C */
    /* TODO: 连接 PT100 测试板后实现实际转换公式 */
    return 25.0f;
}

void Channel_AutoSample_Start(uint8_t interval) {
    auto_interval_s = interval;
    auto_last_tick = g_sys_tick;
    g_sys_state = STATE_AUTO_SAMPLE;
}

void Channel_AutoSample_Stop(void) {
    auto_interval_s = 0;
    g_sys_state = STATE_IDLE;
}

/* 主循环中调用: 检查是否到达采样间隔, 是则发送自动上报帧 */
void Channel_AutoSample_Process(void) {
    if (g_sys_state != STATE_AUTO_SAMPLE || auto_interval_s == 0)
        return;

    /* SysTick 周期 = 2ms (500Hz), 计算经过的毫秒数 */
    uint32_t elapsed_ms = (g_sys_tick - auto_last_tick) * 2;
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
