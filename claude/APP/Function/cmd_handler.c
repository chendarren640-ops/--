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

    /* 广播帧: FFFF=广播地址, 非广播且ID不匹配则丢弃 */
    if (FR_ID != 0xFFFF && FR_ID != g_param.device_id) return;

    /* 自动上报期间仅响应停止命令 (0x0303) */
    if (g_sys_state == STATE_AUTO_SAMPLE && cmd != 0x0303) return;

    /* ================================================================
     * 协议约定: 所有请求帧 TT=0x01, 应答帧 TT=0x02
     * 命令字 CCCC 决定功能分类, 与 TT 无关
     *   0x01xx=系统管理  0x02xx=数据查询  0x03xx=控制
     *   0x04xx=参数配置  0x05xx=OTA升级   0x06xx=告警
     * ================================================================ */
    if (type != 0x01 && type != 0x05) {
        Build_ErrorReply();
        return;
    }

    /* 心跳请求 (TT=0x05, CCCC=FFFF) / 心跳帧回环忽略 (TT=0x05, CCCC=8888) */
    if (type == 0x05) {
        if (cmd == 0xFFFF) { Build_Heartbeat(); }
        /* 0x8888=心跳帧, 可能是TX→RX回环, 静默忽略 */
        return;
    }

    /* 所有命令统一分发 */
    switch (cmd) {

    /* === 0x01xx 系统管理类 === */
    case 0x0101: /* 设备重启 */
        Build_OK_Reply(0x0101);
        delay_1ms(50);
        NVIC_SystemReset();
        break;
    case 0x0104: /* 查询固件版本 → 2.0.1.0 */
        Build_VersionReply();
        break;
    case 0x0105: /* 设置时间 */
        if (FR_LEN >= 4)
            RTC_SetTime(((uint32_t)FR_DATA[0]<<24)|((uint32_t)FR_DATA[1]<<16)
                       |((uint32_t)FR_DATA[2]<<8)|FR_DATA[3]);
        Build_OK_Reply(0x0105);
        break;
    case 0x0106: /* 查询时间 */
        Build_TimeReply(RTC_GetTime());
        break;
    case 0x0111: /* 查询设备 ID (广播) */
        Build_IDReply(g_param.device_id);
        break;
    case 0x0112: /* 查询波特率 */
        Build_BaudReply(g_param.baud_code);
        break;
    case 0x01A1: /* 修改设备 ID — 应答命令字与请求一致 */
        if (FR_LEN >= 2) {
            g_param.device_id = ((uint16_t)FR_DATA[0]<<8) | FR_DATA[1];
            Param_Save();
        }
        Build_OK_Reply(cmd);
        break;
    case 0x01A2: /* 修改波特率 — 回复OK→变更→重启 (赛题: "先回复OK,再变更重启") */
        if (FR_LEN >= 1) {
            g_param.baud_code = FR_DATA[0];
            Param_Save();
            Build_OK_Reply(cmd);          /* 旧波特率发送 OK */
            delay_1ms(500);               /* 等待 OK 帧完全发出 */
            NVIC_SystemReset();           /* 重启 → sys_init 用新波特率初始化 */
        }
        break;

    /* === 0x02xx 数据查询类 === */
    case 0x0201: /* 查询 CH0 */
        Build_DataReply(0x0201, Channel_ReadCH0());
        break;
    case 0x0202: /* 查询 CH1 */
        Build_DataReply(0x0202, Channel_ReadCH1());
        break;
    case 0x0221: /* 查询 CH2 (PT100) */
        Build_DataReply(0x0221, Channel_ReadCH2());
        break;

    /* === 0x03xx 控制类 === */
    case 0x0301: /* 设置 DAC 输出 */
        if (FR_LEN >= 2) DAC_SetValue(((uint16_t)FR_DATA[0]<<8)|FR_DATA[1]);
        Build_OK_Reply(0x0301);
        break;
    case 0x0261: /* 设置上报间隔 (上位机评测用) */
        if (FR_LEN >= 1) Channel_AutoSample_SetInterval(FR_DATA[0]);
        Build_OK_Reply(cmd);
        break;
    case 0x0302: /* 开始自动上报 (默认间隔5s, 与上位机一致) */
        if (FR_LEN >= 1) Channel_AutoSample_Start(FR_DATA[0]);
        else Channel_AutoSample_Start(5);  /* 默认5秒间隔 */
        Build_OK_Reply(0x0302);
        break;
    case 0x0303: /* 停止自动上报 */
        Channel_AutoSample_Stop();
        Build_OK_Reply(0x0303);
        break;
    case 0x03AA: /* 深度睡眠 + RTC 唤醒 (参考: bsp_enter_deepsleep_rtc_wakeup) */
        Build_OK_Reply(0x03AA);
        delay_1ms(50);

        /* 关全局中断 → 进入 PMU 深度睡眠 → RTC 10秒后唤醒 */
        __disable_irq();
        rcu_periph_clock_enable(RCU_PMU);
        pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
        pmu_flag_clear(PMU_FLAG_RESET_STANDBY);
        RTC_WakeupTimer_Start(10);  /* 10 秒后 RTC 唤醒 */
        SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
        __enable_irq();
        pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);

        /* 唤醒后 → 重新初始化 → 发送唤醒消息 */
        System_Init();
        ProtoSendString("instrument wakeup");
        break;

    /* === 0x04xx 参数配置类 === */
    case 0x0241: /* 设置 CH0 变比 */
        if (FR_LEN >= 4) { g_param.ch0_ratio = parse_float_be(FR_DATA); Param_Save(); }
        Build_OK_Reply(cmd);
        break;
    case 0x0242: /* 设置 CH1 变比 */
        if (FR_LEN >= 4) { g_param.ch1_ratio = parse_float_be(FR_DATA); Param_Save(); }
        Build_OK_Reply(cmd);
        break;
    case 0x0401: /* 查询 CH0 阈值 (E-01, F-03 单独读) */
        Build_DataReply(0x0401, g_param.ch0_threshold);
        break;
    case 0x0402: /* 查询 CH1 阈值 (E-02, F-04 单独读) */
        Build_DataReply(0x0402, g_param.ch1_threshold);
        break;
    case 0x0411: /* 设置 CH0 阈值 */
        if (FR_LEN >= 4) { g_param.ch0_threshold = parse_float_be(FR_DATA); Param_Save(); }
        Build_OK_Reply(0x0411);
        break;
    case 0x0412: /* 设置 CH1 阈值 */
        if (FR_LEN >= 4) { g_param.ch1_threshold = parse_float_be(FR_DATA); Param_Save(); }
        Build_OK_Reply(0x0412);
        break;
    case 0x0403: /* 查询 CH2 阈值 (PT100) */
        Build_DataReply(0x0403, g_param.ch2_threshold);
        break;
    case 0x0413: /* 设置 CH2 阈值 (PT100) */
        if (FR_LEN >= 4) { g_param.ch2_threshold = parse_float_be(FR_DATA); Param_Save(); }
        Build_OK_Reply(0x0413);
        break;
    case 0x0400: /* 查询阈值 */
    case 0x0421:
        Build_ThresholdReply(cmd, g_param.ch0_threshold, g_param.ch1_threshold);
        break;

    /* === 0x05xx OTA 升级类 === */
    case 0x0501: /* 升级请求 */
        g_param.upgrade_flag = 0xA5;
        Param_Save();
        Build_OK_Reply(0x0501);
        delay_1ms(50);
        NVIC_SystemReset();
        break;

    /* === 0x06xx 告警类 === */
    case 0x0601: /* 设置告警模式 */
        if (FR_LEN >= 1) Alarm_SetMode(FR_DATA[0]);
        Build_OK_Reply(0x0601);
        break;
    case 0x0602: /* 查询告警记录 (字符串, 不组帧) */
        Alarm_Query();
        break;
    case 0x0603: /* 清除告警 */
        Alarm_Clear();
        Build_OK_Reply(0x0603);
        break;

    default:
        Build_ErrorReply(); /* K-03: 非法命令字 */
        break;
    }
}
