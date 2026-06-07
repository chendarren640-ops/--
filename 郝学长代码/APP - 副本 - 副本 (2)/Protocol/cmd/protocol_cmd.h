#ifndef __PROTOCOL_CMD_H
#define __PROTOCOL_CMD_H

#include <stdint.h>
#include "app_param.h"    // 使用 app_param.h 中的结构体定义
#include "app_sample.h"   // 使用 app_sample.h 中的结构体定义
///* ==================== 数据结构定义（与Function层共享） ==================== */
//// 设备参数结构体（示例，请根据实际调整字段）
//typedef struct {
//    uint16_t device_id;             // 设备ID
//    uint8_t  baudrate;              // 波特率映射值
//    uint8_t  autoreport_interval;   // 上报间隔：1,2,3分别对应1s,3s,5s
//    uint8_t  autoreport_enable;     // 自动上报使能
//    uint8_t  alarm_enable;          // 告警模式
//    float    ch0_ratio;             // CH0变比
//    float    ch1_ratio;             // CH1变比
//    float    ch0_threshold;         // CH0阈值
//    float    ch1_threshold;         // CH1阈值
//} app_param_t;

//// 采样数据结构体（示例）
//typedef struct {
//    float ch0_value;    // CH0原始电压（已乘变比）
//    float ch1_value;    // CH1原始电压（已乘变比）
//    float ch2_temp;     // PT100温度
//} app_sample_t;

/* ==================== 帧类型定义 ==================== */
#define FRAME_TYPE_CMD        0x01   // 上位机 -> 设备：命令下发帧
#define FRAME_TYPE_RESP       0x02   // 设备 -> 上位机：应答帧
#define FRAME_TYPE_HEARTBEAT  0x05   // 双向：心跳帧
#define FRAME_TYPE_ERROR      0xFF   // 设备 -> 上位机：异常/错误应答帧

/* ==================== 命令字定义 ==================== */
// 1. 系统管理类 (0x01xx)
#define CMD_SYS_REBOOT              0x0101
#define CMD_SYS_FACTORY_RESET       0x0102
#define CMD_SYS_DEVICE_INFO         0x0103
#define CMD_SYS_FW_VERSION          0x0104
#define CMD_SYS_SET_TIME            0x0105
#define CMD_SYS_GET_TIME            0x0106
#define CMD_SYS_SET_ID              0x01A1
#define CMD_SYS_SET_BAUDRATE        0x01A2
#define CMD_SYS_GET_ID              0x0111
#define CMD_SYS_GET_BAUDRATE        0x0112

// 2. 数据类 (0x02xx)
#define CMD_DATA_CH0                0x0201
#define CMD_DATA_CH1                0x0202
#define CMD_DATA_CH2                0x0221
#define CMD_DATA_SET_SCALE_CH0      0x0241
#define CMD_DATA_SET_SCALE_CH1      0x0242
#define CMD_DATA_SET_REPORT_INTERVAL 0x0261

// 3. 控制类 (0x03xx)
#define CMD_CTRL_DAC                0x0301
#define CMD_CTRL_AUTO_REPORT_START  0x0302
#define CMD_CTRL_AUTO_REPORT_STOP   0x0303
#define CMD_CTRL_SLEEP              0x03AA

// 4. 参数配置类 (0x04xx)
#define CMD_CFG_THRESHOLD_ALL       0x0400
#define CMD_CFG_THRESHOLD_CH0       0x0401
#define CMD_CFG_THRESHOLD_CH1       0x0402
#define CMD_CFG_THRESHOLD_CH2       0x0403
#define CMD_CFG_WRITE_THRESHOLD_CH0 0x0411
#define CMD_CFG_WRITE_THRESHOLD_CH1 0x0412
#define CMD_CFG_WRITE_THRESHOLD_CH2 0x0413

// 5. 系统升级类 (0x05xx)
#define CMD_UPGRADE_REQUEST         0x0501
#define CMD_UPGRADE_PREPARE         0x0502
#define CMD_UPGRADE_EXECUTE         0x0503

// 6. 告警与日志类 (0x06xx)
#define CMD_ALARM_SET_MODE          0x0601
#define CMD_ALARM_QUERY             0x0602
#define CMD_ALARM_CLEAR             0x0603
#define CMD_ALARM_LOG_QUERY         0x0604
#define CMD_ALARM_LOG_CLEAR         0x0605

// 7. 特殊命令字
#define CMD_HEARTBEAT_UPLINK        0x8888
#define CMD_BROADCAST_FIND          0xFFFF
#define CMD_ERROR_RESP              0xEEEE

/* ==================== 帧格式常量 ==================== */
#define FRAME_HEADER         0xA5B6
#define FRAME_TAIL           0xB6A5
#define PROTOCOL_VERSION     0x02
#define FRAME_MIN_LEN        11

/* ==================== 其他常量 ==================== */
#define BAUDRATE_MAP_4800    0x11
#define BAUDRATE_MAP_9600    0x12
#define BAUDRATE_MAP_19200   0x13
#define BAUDRATE_MAP_115200  0x14

#define REPORT_INTERVAL_1S   0x01
#define REPORT_INTERVAL_3S   0x02
#define REPORT_INTERVAL_5S   0x03

#define ALARM_MODE_ACTIVE    0x01
#define ALARM_MODE_PASSIVE   0x02

/* ==================== 外部变量声明 ==================== */
extern app_param_t g_app_param;          // 全局参数结构体（由Function层定义）
extern app_sample_t g_latest_sample;     // 最新采样数据

/* ==================== 函数声明 ==================== */
void Protocol_ReceiveByte(uint8_t byte);
void Protocol_Process(void);

#endif /* __PROTOCOL_CMD_H */

