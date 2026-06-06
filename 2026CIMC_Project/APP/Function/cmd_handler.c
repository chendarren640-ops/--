#include "cmd_handler.h"
#include "frame_builder.h"
#include "flash_param.h"
#include "adc_drv.h"
#include "dac_drv.h"
#include "rtc_drv.h"

/* 帧[0-1]=0xA5B6, [2-3]=设备ID, [4]=帧类型, [5-6]=命令字, [7]=长度, [8]=版本, [9..]=内容 */
#define FR_ID     (((uint16_t)(frame[2])<<8)|frame[3])
#define FR_TYPE   frame[4]
#define FR_CMD    (((uint16_t)(frame[5])<<8)|frame[6])
#define FR_LEN    frame[7]
#define FR_DATA   (&frame[9])

void cmd_dispatch(uint8_t *frame, uint16_t len) {
    uint8_t  type = FR_TYPE;
    uint16_t cmd  = FR_CMD;

    /* 广播帧: FFFF=广播地址, 非广播且ID不匹配则丢弃 */
    if (FR_ID != 0xFFFF && FR_ID != g_param.device_id) return;

    switch (type) {
    case 0x01: /* 命令下发帧 */
        switch (cmd) {
        case 0x0101: /* 设备重启 */
            Build_OK_Reply(0x0101); NVIC_SystemReset(); break;
        case 0x0104: /* 查询固件版本 */
            Build_VersionReply(); break;
        case 0x0105: /* 设置时间 */
            if (FR_LEN>=4) RTC_SetTime((FR_DATA[0]<<24)|(FR_DATA[1]<<16)|(FR_DATA[2]<<8)|FR_DATA[3]);
            Build_OK_Reply(0x0105); break;
        case 0x0106: /* 查询时间 */
            Build_TimeReply(RTC_GetTime()); break;
        case 0x01A1: /* 设置ID */
            if (FR_LEN>=2) { g_param.device_id=(FR_DATA[0]<<8)|FR_DATA[1]; Param_Save(); }
            Build_IDReply(g_param.device_id); break;
        case 0x01A2: /* 设置波特率 */
            if (FR_LEN>=1) { g_param.baud_code=FR_DATA[0]; Param_Save(); Build_OK_Reply(0x01A2); USART1_Config_Baud(115200); } break;
        case 0x0111: /* 查询ID (广播地址) */
            Build_IDReply(g_param.device_id); break;
        case 0x0112: /* 查询波特率 */
            Build_BaudReply(g_param.baud_code); break;
        }
        break;
    case 0x05: /* 心跳帧 */
        if (cmd==0xFFFF) Build_Heartbeat();
        break;
    }
    /* TODO: 数据类0x02, 控制类0x03, 参数配置类0x04, 升级类0x05, 告警类0x06 */
}
