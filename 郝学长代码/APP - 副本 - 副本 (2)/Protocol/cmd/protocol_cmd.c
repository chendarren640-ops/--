/*
 * protocol_cmd.c
 * 通讯协议层 - 帧解析、应答组帧、命令处理
 */

#include "protocol_cmd.h"
#include "protocol_crc.h"
#include "protocol_ascii_hex.h"
#include "app_param.h"
#include "app_sample.h"
#include "app_alarm.h"
#include "app_autoreport.h"
#include "app_sleep.h"
#include "bsp_usart_rs485.h"
#include "bsp_flash.h"
#include "bsp_rtc.h"
#include "bsp_dac.h"
#include "systick.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ==================== 外部函数声明 ==================== */
/* 使用 CMSIS 系统复位函数 */
#define system_soft_reset()  NVIC_SystemReset()
extern uint32_t app_millis(void);
extern void APP_Sample_GetLatest(app_sample_t *sample);
extern uint8_t APP_Param_Save(void);
extern void BSP_DAC_SetVoltage(float voltage);
extern void APP_Sleep_Enter10s(void);
extern uint8_t APP_Get_Alarm_Records(char *buf, uint16_t buf_size);
extern void APP_Alarm_Clear(void);
extern void APP_Alarm_SetMode(uint8_t mode);
extern void RTC_GetDateTimeString(char *buf, uint8_t len);
extern void bsp_rtc_set_unix_timestamp(uint32_t ts);
extern uint32_t bsp_rtc_get_unix_timestamp(void);

/* ==================== 内部变量 ==================== */
static uint8_t rx_buffer[1024];
static uint16_t rx_index = 0;
static uint8_t frame_ready = 0;

/* ==================== 发送辅助函数 ==================== */

/* 发送 ASCII 字符串 (非帧封装) */
static void send_ascii_str(const char *str)
{
    BSP_RS485_SendData((const uint8_t *)str, strlen(str));
}

/* 将二进制帧转换为 ASCII HEX 并发送 */
static void send_frame_as_ascii(const uint8_t *frame, uint16_t len)
{
    uint8_t ascii_buf[512];
    int ascii_len = bytes_to_ascii_hex(frame, len, ascii_buf, sizeof(ascii_buf));
    if (ascii_len > 0)
    {
        BSP_RS485_SendData(ascii_buf, ascii_len);
    }
}

/* ==================== 应答帧组帧 ==================== */

/*
 * 组装并发送应答帧
 * frame_type=0x02, 命令字=cmd, 内容=content[content_len]
 */
static void protocol_send_response(uint16_t device_id, uint16_t cmd,
                                   const uint8_t *content, uint16_t content_len)
{
    uint8_t frame[256];
    uint16_t idx = 0;
    uint16_t crc;

    /* 起始标志 */
    frame[idx++] = 0xA5;
    frame[idx++] = 0xB6;
    /* 设备 ID */
    frame[idx++] = (device_id >> 8) & 0xFF;
    frame[idx++] = device_id & 0xFF;
    /* 帧类型: 应答 */
    frame[idx++] = 0x02;
    /* 命令字 */
    frame[idx++] = (cmd >> 8) & 0xFF;
    frame[idx++] = cmd & 0xFF;
    /* 报文长度 */
    frame[idx++] = (uint8_t)content_len;
    /* 协议版本 */
    frame[idx++] = PROTOCOL_VERSION;
    /* 内容 */
    if (content != NULL && content_len > 0)
    {
        memcpy(frame + idx, content, content_len);
        idx += content_len;
    }
    /* CRC16 */
    crc = Protocol_CRC16_Modbus(frame, idx);
    frame[idx++] = (crc >> 8) & 0xFF;
    frame[idx++] = crc & 0xFF;
    /* 结束标志 */
    frame[idx++] = 0xB6;
    frame[idx++] = 0xA5;

    send_frame_as_ascii(frame, idx);
}

/* 发送错误应答帧 (帧类型 0xFF, 命令字 0xEEEE) */
static void send_error_frame(uint16_t device_id)
{
    uint8_t frame[256];
    uint16_t idx = 0;
    uint16_t crc;

    frame[idx++] = 0xA5;
    frame[idx++] = 0xB6;
    frame[idx++] = (device_id >> 8) & 0xFF;
    frame[idx++] = device_id & 0xFF;
    frame[idx++] = 0xFF;                    /* 帧类型: 错误 */
    frame[idx++] = 0xEE;
    frame[idx++] = 0xEE;                    /* 命令字 0xEEEE */
    frame[idx++] = 0x00;                    /* 内容长度 0 */
    frame[idx++] = PROTOCOL_VERSION;

    crc = Protocol_CRC16_Modbus(frame, idx);
    frame[idx++] = (crc >> 8) & 0xFF;
    frame[idx++] = crc & 0xFF;
    frame[idx++] = 0xB6;
    frame[idx++] = 0xA5;

    send_frame_as_ascii(frame, idx);
}

/* 发送心跳帧 (帧类型 0x05, 命令字 0x8888) */
static void send_heartbeat_frame(void)
{
    uint8_t frame[256];
    uint16_t idx = 0;
    uint16_t crc;
    uint16_t device_id = g_app_param.device_id;

    frame[idx++] = 0xA5;
    frame[idx++] = 0xB6;
    frame[idx++] = (device_id >> 8) & 0xFF;
    frame[idx++] = device_id & 0xFF;
    frame[idx++] = 0x05;
    frame[idx++] = 0x88;
    frame[idx++] = 0x88;
    frame[idx++] = 0x00;
    frame[idx++] = PROTOCOL_VERSION;

    crc = Protocol_CRC16_Modbus(frame, idx);
    frame[idx++] = (crc >> 8) & 0xFF;
    frame[idx++] = crc & 0xFF;
    frame[idx++] = 0xB6;
    frame[idx++] = 0xA5;

    send_frame_as_ascii(frame, idx);
}

/* float 转大端字节序 (用于发送) */
static void float_to_big_endian(float f, uint8_t *buf)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    buf[0] = (u >> 24) & 0xFF;
    buf[1] = (u >> 16) & 0xFF;
    buf[2] = (u >> 8) & 0xFF;
    buf[3] = u & 0xFF;
}

/* 大端字节序转 float (用于接收) */
static float big_endian_to_float(const uint8_t *buf)
{
    uint32_t u = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
                 ((uint32_t)buf[2] << 8) | buf[3];
    float f;
    memcpy(&f, &u, 4);
    return f;
}

/* ==================== 命令处理 ==================== */

static void process_frame(uint8_t *bin, uint16_t len)
{
    uint16_t device_id, cmd, crc_calc, crc_recv;
    uint8_t frame_type, length_field;
    uint8_t *content;
    uint16_t content_len;

    /* 最小帧长度检查 (头+ID+类型+命令+长度+版本+CRC+尾 = 13) */
    if (len < 13)
    {
        send_error_frame(g_app_param.device_id);
        return;
    }

    /* 解析字段 */
    device_id    = ((uint16_t)bin[2] << 8) | bin[3];
    frame_type   = bin[4];
    cmd          = ((uint16_t)bin[5] << 8) | bin[6];
    length_field = bin[7];
    content      = bin + 9;
    content_len  = length_field;

    /* 总长度校验 */
    if (9 + content_len + 4 != len)
    {
        send_error_frame(device_id);
        return;
    }

    /* 结束标志校验 */
    if (!(bin[len - 2] == 0xB6 && bin[len - 1] == 0xA5))
    {
        send_error_frame(device_id);
        return;
    }

    /* CRC 校验 */
    crc_calc = Protocol_CRC16_Modbus(bin, 9 + content_len);
    crc_recv = ((uint16_t)bin[9 + content_len] << 8) | bin[9 + content_len + 1];
    if (crc_calc != crc_recv)
    {
        send_error_frame(device_id);
        return;
    }

    /* 设备 ID 检查: 广播 0xFFFF 或匹配本机 */
    if (device_id != 0xFFFF && device_id != g_app_param.device_id)
    {
        return;  /* 静默丢弃 */
    }

    /* 帧类型检查: 只接受命令下发帧(0x01)和广播/心跳帧(0x05) */
    if (frame_type != 0x01 && frame_type != 0x05)
    {
        send_error_frame(device_id);
        return;
    }

    /* 广播寻找设备帧 (帧类型 0x05, 命令字 0xFFFF) */
    if (frame_type == 0x05 && cmd == CMD_BROADCAST_FIND)
    {
        send_heartbeat_frame();
        return;
    }

    /* 其他心跳帧直接忽略 */
    if (frame_type == 0x05)
    {
        return;
    }

    /* ============ 自动上报期间拒绝非停止命令 (H-02) ============ */
    if (g_app_param.autoreport_enable && cmd != CMD_CTRL_AUTO_REPORT_STOP)
    {
        send_error_frame(g_app_param.device_id);
        return;
    }

    /* ============ 命令分发处理 ============ */
    uint8_t resp_content[128];
    uint16_t resp_len = 0;

    switch (cmd)
    {
        /* ========== 系统管理类 ========== */

        case CMD_SYS_REBOOT:
        {
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            delay_1ms(50);
            system_soft_reset();
            break;
        }

        case CMD_SYS_FW_VERSION:
        {
            /* 版本号 2.0.1.0 */
            resp_content[0] = (APP_VERSION_NUM >> 24) & 0xFF;
            resp_content[1] = (APP_VERSION_NUM >> 16) & 0xFF;
            resp_content[2] = (APP_VERSION_NUM >> 8) & 0xFF;
            resp_content[3] = APP_VERSION_NUM & 0xFF;
            resp_len = 4;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_SYS_SET_TIME:
        {
            if (content_len != 4) { send_error_frame(g_app_param.device_id); break; }
            uint32_t timestamp = ((uint32_t)content[0] << 24) | ((uint32_t)content[1] << 16) |
                                 ((uint32_t)content[2] << 8) | content[3];
            bsp_rtc_set_unix_timestamp(timestamp);
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_SYS_GET_TIME:
        {
            uint32_t now_ts = bsp_rtc_get_unix_timestamp();
            resp_content[0] = (now_ts >> 24) & 0xFF;
            resp_content[1] = (now_ts >> 16) & 0xFF;
            resp_content[2] = (now_ts >> 8) & 0xFF;
            resp_content[3] = now_ts & 0xFF;
            resp_len = 4;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_SYS_SET_ID:
        {
            if (content_len != 2) { send_error_frame(g_app_param.device_id); break; }
            uint16_t new_id = ((uint16_t)content[0] << 8) | content[1];
            if (new_id == 0x0000 || new_id == 0xFFFF) { send_error_frame(g_app_param.device_id); break; }
            g_app_param.device_id = new_id;
            APP_Param_Save();
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_SYS_SET_BAUDRATE:
        {
            if (content_len != 1) { send_error_frame(g_app_param.device_id); break; }
            uint8_t map_val = content[0];
            if (map_val < 0x11 || map_val > 0x14) { send_error_frame(g_app_param.device_id); break; }
            g_app_param.baudrate = map_val;
            APP_Param_Save();
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            /* 先回复 OK，再切换波特率 */
            delay_1ms(10);
            BSP_RS485_SetBaudrate(APP_Param_GetBaudrateValue(map_val));
            break;
        }

        case CMD_SYS_GET_ID:
        {
            resp_content[0] = (g_app_param.device_id >> 8) & 0xFF;
            resp_content[1] = g_app_param.device_id & 0xFF;
            resp_len = 2;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_SYS_GET_BAUDRATE:
        {
            resp_content[0] = g_app_param.baudrate;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        /* ========== 数据类 ========== */

        case CMD_DATA_CH0:
        {
            app_sample_t sample;
            APP_Sample_GetLatest(&sample);
            float_to_big_endian(sample.ch0_value, resp_content);
            resp_len = 4;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_DATA_CH1:
        {
            app_sample_t sample;
            APP_Sample_GetLatest(&sample);
            float_to_big_endian(sample.ch1_value, resp_content);
            resp_len = 4;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_DATA_CH2:
        {
            app_sample_t sample;
            APP_Sample_GetLatest(&sample);
            float_to_big_endian(sample.ch2_temp, resp_content);
            resp_len = 4;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_DATA_SET_SCALE_CH0:
        {
            if (content_len != 4) { send_error_frame(g_app_param.device_id); break; }
            g_app_param.ch0_ratio = big_endian_to_float(content);
            APP_Param_Save();
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_DATA_SET_SCALE_CH1:
        {
            if (content_len != 4) { send_error_frame(g_app_param.device_id); break; }
            g_app_param.ch1_ratio = big_endian_to_float(content);
            APP_Param_Save();
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_DATA_SET_REPORT_INTERVAL:
        {
            if (content_len != 1) { send_error_frame(g_app_param.device_id); break; }
            uint8_t interval = content[0];
            if (interval < 1 || interval > 3) interval = 1;
            g_app_param.autoreport_interval = interval;
            APP_Param_Save();
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        /* ========== 控制类 ========== */

        case CMD_CTRL_DAC:
        {
            if (content_len != 2) { send_error_frame(g_app_param.device_id); break; }
            uint16_t dac_val = ((uint16_t)content[0] << 8) | content[1];
            float voltage = (float)dac_val * 3.3f / 4095.0f;
            BSP_DAC_SetVoltage(voltage);
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_CTRL_AUTO_REPORT_START:
        {
            g_app_param.autoreport_enable = 1;
            APP_Param_Save();

            /* 发送第一帧数据 (时间戳 + CH0 + CH1) */
            {
                app_sample_t sample;
                APP_Sample_GetLatest(&sample);
                uint32_t now_ts = bsp_rtc_get_unix_timestamp();
                uint8_t data[12];
                data[0] = (now_ts >> 24) & 0xFF;
                data[1] = (now_ts >> 16) & 0xFF;
                data[2] = (now_ts >> 8) & 0xFF;
                data[3] = now_ts & 0xFF;
                float_to_big_endian(sample.ch0_value, data + 4);
                float_to_big_endian(sample.ch1_value, data + 8);
                protocol_send_response(g_app_param.device_id, cmd, data, 12);
            }
            break;
        }

        case CMD_CTRL_AUTO_REPORT_STOP:
        {
            g_app_param.autoreport_enable = 0;
            APP_Param_Save();
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_CTRL_SLEEP:
        {
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            delay_1ms(10);
            APP_Sleep_Enter10s();
            break;
        }

        /* ========== 参数配置类 ========== */

        case CMD_CFG_THRESHOLD_ALL:
        {
            float_to_big_endian(g_app_param.ch0_threshold, resp_content);
            float_to_big_endian(g_app_param.ch1_threshold, resp_content + 4);
            resp_len = 8;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_CFG_THRESHOLD_CH0:
        {
            float_to_big_endian(g_app_param.ch0_threshold, resp_content);
            resp_len = 4;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_CFG_THRESHOLD_CH1:
        {
            float_to_big_endian(g_app_param.ch1_threshold, resp_content);
            resp_len = 4;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_CFG_THRESHOLD_CH2:
        {
            float zero = 0.0f;
            float_to_big_endian(zero, resp_content);
            resp_len = 4;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_CFG_WRITE_THRESHOLD_CH0:
        {
            if (content_len != 4) { send_error_frame(g_app_param.device_id); break; }
            g_app_param.ch0_threshold = big_endian_to_float(content);
            APP_Param_Save();
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_CFG_WRITE_THRESHOLD_CH1:
        {
            if (content_len != 4) { send_error_frame(g_app_param.device_id); break; }
            g_app_param.ch1_threshold = big_endian_to_float(content);
            APP_Param_Save();
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        case CMD_CFG_WRITE_THRESHOLD_CH2:
        {
            send_error_frame(g_app_param.device_id);
            break;
        }

        /* ========== 系统升级类 ========== */

        case CMD_UPGRADE_REQUEST:
        {
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            delay_1ms(50);
            system_soft_reset();  /* 重启到 Bootloader */
            break;
        }

        case CMD_UPGRADE_PREPARE:
        case CMD_UPGRADE_EXECUTE:
        {
            /* APP 中不应处理这些命令 */
            send_error_frame(g_app_param.device_id);
            break;
        }

        /* ========== 告警与日志类 ========== */

        case CMD_ALARM_SET_MODE:
        {
            if (content_len != 1) { send_error_frame(g_app_param.device_id); break; }
            if (content[0] == ALARM_MODE_ACTIVE || content[0] == ALARM_MODE_PASSIVE)
            {
                g_app_param.alarm_enable = content[0];
                APP_Alarm_SetMode(content[0]);
                APP_Param_Save();
                resp_content[0] = 0xFF;
                resp_len = 1;
                protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            }
            else
                send_error_frame(g_app_param.device_id);
            break;
        }

        case CMD_ALARM_QUERY:
        {
            char alarm_buf[512];
            uint16_t cnt = APP_Get_Alarm_Records(alarm_buf, sizeof(alarm_buf));
            if (cnt == 0)
                send_ascii_str("empty");
            else
                send_ascii_str(alarm_buf);
            break;
        }

        case CMD_ALARM_CLEAR:
        {
            APP_Alarm_Clear();
            resp_content[0] = 0xFF;
            resp_len = 1;
            protocol_send_response(g_app_param.device_id, cmd, resp_content, resp_len);
            break;
        }

        default:
            send_error_frame(g_app_param.device_id);
            break;
    }
}

/* ==================== 外部接口 ==================== */

/*
 * 串口接收字节回调 (在中断中调用)
 * 检测 ASCII "B6A5" 作为帧结束标志
 */
void Protocol_ReceiveByte(uint8_t byte)
{
    if (rx_index < sizeof(rx_buffer) - 1)
    {
        rx_buffer[rx_index++] = byte;
        /* 检测帧尾: ASCII "B6A5" */
        if (rx_index >= 4 &&
            rx_buffer[rx_index - 4] == 'B' && rx_buffer[rx_index - 3] == '6' &&
            rx_buffer[rx_index - 2] == 'A' && rx_buffer[rx_index - 1] == '5')
        {
            frame_ready = 1;
        }
    }
    else
    {
        rx_index = 0; /* 溢出重置 */
    }
}

/*
 * 协议处理 (在主循环中调用)
 */
void Protocol_Process(void)
{
    if (frame_ready)
    {
        frame_ready = 0;
        uint8_t bin[512];
        int bin_len = ascii_hex_to_bytes(rx_buffer, rx_index, bin, sizeof(bin));
        rx_index = 0;
        if (bin_len > 0)
        {
            process_frame(bin, (uint16_t)bin_len);
        }
        else
        {
            send_error_frame(g_app_param.device_id);
        }
    }
}
