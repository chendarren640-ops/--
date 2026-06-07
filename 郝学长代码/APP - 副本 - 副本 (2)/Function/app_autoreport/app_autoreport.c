/*
 * app_autoreport.c
 * 自动上报模块 - 按设定间隔发送二进制协议帧
 * 帧格式: 时间戳(4B) + CH0数据(4B, IEEE754) + CH1数据(4B, IEEE754) = 12字节
 */

#include "app_autoreport.h"
#include "app_param.h"
#include "app_sample.h"
#include "bsp_usart_rs485.h"
#include "bsp_rtc.h"
#include "protocol_ascii_hex.h"
#include "protocol_crc.h"
#include <string.h>

extern uint32_t app_millis(void);

static uint32_t s_report_tick = 0;

/* 将 float 转换为大端字节序 */
static void float_to_big_endian(float f, uint8_t *buf)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    buf[0] = (u >> 24) & 0xFF;
    buf[1] = (u >> 16) & 0xFF;
    buf[2] = (u >> 8) & 0xFF;
    buf[3] = u & 0xFF;
}

void APP_AutoReport_Init(void)
{
    s_report_tick = 0;
}

/*
 * 自动上报任务 - 在主循环中调用
 * 按协议格式发送二进制帧: 时间戳 + CH0 + CH1 (均乘以变比)
 */
void APP_AutoReport_Task(void)
{
    app_sample_t sample;
    uint8_t frame[64];
    uint8_t ascii_buf[128];
    uint16_t idx = 0;
    uint16_t crc;
    uint32_t now_ts;
    uint32_t interval_ms;

    /* 1. 检查自动上报是否使能 */
    if (g_app_param.autoreport_enable == 0)
    {
        return;
    }

    /* 2. 时间间隔检查 */
    interval_ms = APP_Param_GetIntervalMs(g_app_param.autoreport_interval);
    uint32_t now = app_millis();
    if (now - s_report_tick < interval_ms)
    {
        return;
    }
    s_report_tick = now;

    /* 3. 采样 */
    APP_Sample_GetLatest(&sample);

    /* 4. 获取当前时间戳 */
    now_ts = bsp_rtc_get_unix_timestamp();

    /* 5. 组帧: 协议格式的自动上报帧 */
    /* 帧头 */
    frame[idx++] = 0xA5;
    frame[idx++] = 0xB6;
    /* 设备ID */
    frame[idx++] = (g_app_param.device_id >> 8) & 0xFF;
    frame[idx++] = g_app_param.device_id & 0xFF;
    /* 帧类型: 0x02 (应答帧) */
    frame[idx++] = 0x02;
    /* 命令字: 0x0302 */
    frame[idx++] = 0x03;
    frame[idx++] = 0x02;
    /* 报文长度: 12字节 */
    frame[idx++] = 0x0C;
    /* 协议版本 */
    frame[idx++] = 0x02;
    /* 内容: 时间戳(4B) + CH0(4B) + CH1(4B) */
    frame[idx++] = (now_ts >> 24) & 0xFF;
    frame[idx++] = (now_ts >> 16) & 0xFF;
    frame[idx++] = (now_ts >> 8) & 0xFF;
    frame[idx++] = now_ts & 0xFF;
    float_to_big_endian(sample.ch0_value, frame + idx);
    idx += 4;
    float_to_big_endian(sample.ch1_value, frame + idx);
    idx += 4;
    /* CRC16 */
    crc = Protocol_CRC16_Modbus(frame, idx);
    frame[idx++] = (crc >> 8) & 0xFF;
    frame[idx++] = crc & 0xFF;
    /* 帧尾 */
    frame[idx++] = 0xB6;
    frame[idx++] = 0xA5;

    /* 6. 转换为 ASCII HEX 并发送 */
    int ascii_len = bytes_to_ascii_hex(frame, idx, ascii_buf, sizeof(ascii_buf));
    if (ascii_len > 0)
    {
        BSP_RS485_SendData(ascii_buf, ascii_len);
    }
}
