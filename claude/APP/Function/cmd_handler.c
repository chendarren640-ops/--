#include "cmd_handler.h"
#include "frame_builder.h"
#include "flash_param.h"
#include "adc_drv.h"
#include "dac_drv.h"
#include "rtc_drv.h"
#include "data_channel.h"
#include "alarm_mgr.h"

/* 帧[0-1]=0xA5B6, [2-3]=设备ID, [4]=帧类型, [5-6]=命令字, [7]=长度, [8]=版本, [9..]=内容 */
#define FR_ID     (((uint16_t)(frame[2])<<8)|frame[3])
#define FR_TYPE   frame[4]
#define FR_CMD    (((uint16_t)(frame[5])<<8)|frame[6])
#define FR_LEN    frame[7]
#define FR_DATA   (&frame[9])

/* 从帧数据中解析 IEEE 754 浮点数 (大端序) */
static float parse_float_be(uint8_t *data) {
    uint32_t raw = ((uint32_t)data[0]<<24) | ((uint32_t)data[1]<<16)
                 | ((uint32_t)data[2]<<8)  |  (uint32_t)data[3];
    float val;
    memcpy(&val, &raw, 4);
    return val;
}

void cmd_dispatch(uint8_t *frame, uint16_t len) {
    uint8_t  type = FR_TYPE;
    uint16_t cmd  = FR_CMD;

    /* 广播帧: FFFF=广播地址, 非广播且ID不匹配则丢弃 (已在上层校验, 此处防御) */
    if (FR_ID != 0xFFFF && FR_ID != g_param.device_id) return;

    /* 自动上报期间仅响应停止命令 (0x0303) */
    if (g_sys_state == STATE_AUTO_SAMPLE && cmd != 0x0303) return;

    switch (type) {

    /* ================================================================
     * 0x01 — 系统管理类命令 (A/B/C/L/M 模块)
     * ================================================================ */
    case 0x01:
        switch (cmd) {
        case 0x0101: /* 设备重启 (A-01) */
            Build_OK_Reply(0x0101);
            delay_1ms(50);
            NVIC_SystemReset();
            break;
        case 0x0104: /* 查询固件版本 (B-01) → 2.0.1.0 */
            Build_VersionReply();
            break;
        case 0x0105: /* 设置时间 (C-01) */
            if (FR_LEN >= 4)
                RTC_SetTime(((uint32_t)FR_DATA[0]<<24)|((uint32_t)FR_DATA[1]<<16)
                           |((uint32_t)FR_DATA[2]<<8)|FR_DATA[3]);
            Build_OK_Reply(0x0105);
            break;
        case 0x0106: /* 查询时间 (B-02) */
            Build_TimeReply(RTC_GetTime());
            break;
        case 0x0111: /* 查询设备 ID (A-03, 广播) */
            Build_IDReply(g_param.device_id);
            break;
        case 0x0112: /* 查询波特率 (B-03) → 13=19200, 14=115200 */
            Build_BaudReply(g_param.baud_code);
            break;
        case 0x01A1: /* 修改设备 ID (L-01) */
            if (FR_LEN >= 2) {
                g_param.device_id = ((uint16_t)FR_DATA[0]<<8) | FR_DATA[1];
                Param_Save();
            }
            Build_IDReply(g_param.device_id);
            break;
        case 0x01A2: /* 修改波特率 (M-01) */
            if (FR_LEN >= 1) {
                g_param.baud_code = FR_DATA[0];
                Param_Save();
                Build_OK_Reply(0x01A2);
                delay_1ms(10);
                /* 切换到新波特率 (19200→115200) */
                uint32_t new_baud = (FR_DATA[0] == 14) ? 115200UL : 19200UL;
                USART0_DBG_ReconfigBaud(new_baud);
            }
            break;
        default:
            Build_ErrorReply(); /* 非法命令字 (K-03) */
            break;
        }
        break;

    /* ================================================================
     * 0x02 — 数据查询类命令 (B/D 模块)
     * ================================================================ */
    case 0x02:
        switch (cmd) {
        case 0x0201: /* 查询 CH0 值 (B-04) — 电位器 × 变比 */
            Build_DataReply(0x0201, Channel_ReadCH0());
            break;
        case 0x0202: /* 查询 CH1 值 (B-05) — DAC 回读 × 变比 */
            Build_DataReply(0x0202, Channel_ReadCH1());
            break;
        case 0x0221: /* 查询 CH2 值 (B-06) — PT100 温度 */
            Build_DataReply(0x0221, Channel_ReadCH2());
            break;
        default:
            Build_ErrorReply();
            break;
        }
        break;

    /* ================================================================
     * 0x03 — 控制类命令 (D/E/H/J 模块)
     * ================================================================ */
    case 0x03:
        switch (cmd) {
        case 0x0301: /* 设置 DAC 输出 (D-00) */
            if (FR_LEN >= 4) {
                float v = parse_float_be(FR_DATA);
                DAC_SetVoltage(v);
            }
            Build_OK_Reply(0x0301);
            break;
        case 0x0302: /* 开始自动上报 (H-01) */
            if (FR_LEN >= 1) {
                Channel_AutoSample_Start(FR_DATA[0]);
            }
            Build_OK_Reply(0x0302);
            break;
        case 0x0303: /* 停止自动上报 (H-03) */
            Channel_AutoSample_Stop();
            Build_OK_Reply(0x0303);
            break;
        case 0x03AA: /* 深度睡眠 (J-01) */
            Build_OK_Reply(0x03AA);
            delay_1ms(50);
            /* 配置 RTC 闹钟 10 秒后唤醒 */
            RTC_SetAlarm(10);
            /* 进入深度睡眠 */
            pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);
            /* 唤醒后发送提示字符串 (不组帧) */
            USART0_DBG_SendString("instrument wakeup");
            break;
        default:
            Build_ErrorReply();
            break;
        }
        break;

    /* ================================================================
     * 0x04 — 参数配置类命令 (D/E/F 模块)
     * ================================================================ */
    case 0x04:
        switch (cmd) {
        case 0x0401: /* 设置 CH0 变比 (D-01) */
            if (FR_LEN >= 4) {
                g_param.ch0_ratio = parse_float_be(FR_DATA);
                Param_Save();
            }
            Build_OK_Reply(0x0401);
            break;
        case 0x0402: /* 设置 CH1 变比 (D-02) */
            if (FR_LEN >= 4) {
                g_param.ch1_ratio = parse_float_be(FR_DATA);
                Param_Save();
            }
            Build_OK_Reply(0x0402);
            break;
        case 0x0411: /* 设置 CH0 阈值 (E-01) */
            if (FR_LEN >= 4) {
                g_param.ch0_threshold = parse_float_be(FR_DATA);
                Param_Save();
            }
            Build_OK_Reply(0x0411);
            break;
        case 0x0412: /* 设置 CH1 阈值 (E-02) */
            if (FR_LEN >= 4) {
                g_param.ch1_threshold = parse_float_be(FR_DATA);
                Param_Save();
            }
            Build_OK_Reply(0x0412);
            break;
        case 0x0421: /* 查询阈值 (E-03) */
            Build_ThresholdReply(g_param.ch0_threshold, g_param.ch1_threshold);
            break;
        default:
            Build_ErrorReply();
            break;
        }
        break;

    /* ================================================================
     * 0x05 — OTA 升级类命令 (N 模块)
     * ================================================================ */
    case 0x05:
        switch (cmd) {
        case 0x0501: /* 升级请求 (N-01) */
            g_param.upgrade_flag = 0xA5;
            Param_Save();
            Build_OK_Reply(0x0501);
            delay_1ms(50);
            NVIC_SystemReset();  /* 重启进入 Bootloader */
            break;
        case 0xFFFF: /* 心跳帧 (系统自动发送, 无需应答) */
            Build_Heartbeat();
            break;
        default:
            Build_ErrorReply();
            break;
        }
        break;

    /* ================================================================
     * 0x06 — 告警类命令 (I 模块)
     * ================================================================ */
    case 0x06:
        switch (cmd) {
        case 0x0601: /* 设置告警模式 (I-04) */
            if (FR_LEN >= 1) {
                Alarm_SetMode(FR_DATA[0]);
            }
            Build_OK_Reply(0x0601);
            break;
        case 0x0602: /* 查询告警记录 (I-02) — 字符串输出, 不组帧 */
            Alarm_Query();
            break;
        case 0x0603: /* 清除告警记录 (I-03) */
            Alarm_Clear();
            Build_OK_Reply(0x0603);
            break;
        default:
            Build_ErrorReply();
            break;
        }
        break;

    /* ================================================================
     * 未知帧类型 → 异常帧 (K-03)
     * ================================================================ */
    default:
        Build_ErrorReply();
        break;
    }
}
