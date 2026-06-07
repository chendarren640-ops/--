#include "cmd_handler.h"
#include "frame_builder.h"
#include "flash_param.h"
#include "adc_drv.h"
#include "dac_drv.h"
#include "rtc_drv.h"
#include "data_channel.h"
#include "alarm_mgr.h"

#define FR_ID     (((uint16_t)(frame[2])<<8)|frame[3])
#define FR_TYPE   frame[4]
#define FR_CMD    (((uint16_t)(frame[5])<<8)|frame[6])
#define FR_LEN    frame[7]
#define FR_DATA   (&frame[9])

static float parse_float_be(uint8_t *data) {
    uint32_t raw = ((uint32_t)data[0]<<24)|((uint32_t)data[1]<<16)
                 |((uint32_t)data[2]<<8)|(uint32_t)data[3];
    float val; memcpy(&val, &raw, 4); return val;
}

void cmd_dispatch(uint8_t *frame, uint16_t len) {
    uint8_t  type = FR_TYPE;
    uint16_t cmd  = FR_CMD;

    if (FR_ID != 0xFFFF && FR_ID != g_param.device_id) return;
    if (g_sys_state == STATE_AUTO_SAMPLE && cmd != 0x0303) return;

    /* 上位机所有命令用 TT=0x01, 心跳请求用 TT=0x05 */
    if (type != 0x01 && type != 0x05) { Build_ErrorReply(); return; }

    if (type == 0x05) { if (cmd == 0xFFFF) Build_Heartbeat(); return; }

    switch (cmd) {
    /* 系统管理 */
    case 0x0101: Build_OK_Reply(0x0101); delay_1ms(50); NVIC_SystemReset(); break;
    case 0x0104: Build_VersionReply(); break;
    case 0x0105: if(FR_LEN>=4) RTC_SetTime(((uint32_t)FR_DATA[0]<<24)|((uint32_t)FR_DATA[1]<<16)|((uint32_t)FR_DATA[2]<<8)|FR_DATA[3]); Build_OK_Reply(0x0105); break;
    case 0x0106: Build_TimeReply(RTC_GetTime()); break;
    case 0x0111: Build_IDReply(g_param.device_id); break;
    case 0x0112: Build_BaudReply(g_param.baud_code); break;
    case 0x01A1: if(FR_LEN>=2){g_param.device_id=((uint16_t)FR_DATA[0]<<8)|FR_DATA[1];Param_Save();delay_1ms(10);} Build_OK_Reply(cmd); break;
    case 0x01A2: if(FR_LEN>=1){g_param.baud_code=FR_DATA[0];Param_Save();Build_OK_Reply(cmd);delay_1ms(500);NVIC_SystemReset();} break;

    /* 数据查询 */
    case 0x0201: Build_DataReply(0x0201, Channel_ReadCH0()); break;
    case 0x0202: Build_DataReply(0x0202, Channel_ReadCH1()); break;
    case 0x0221: Build_DataReply(0x0221, Channel_ReadCH2()); break;

    /* 变比设置 */
    case 0x0241: if(FR_LEN>=4){g_param.ch0_ratio=parse_float_be(FR_DATA);Param_Save();} Build_OK_Reply(cmd); break;
    case 0x0242: if(FR_LEN>=4){g_param.ch1_ratio=parse_float_be(FR_DATA);Param_Save();} Build_OK_Reply(cmd); break;

    /* 控制 */
    case 0x0301: if(FR_LEN>=2) DAC_SetValue(((uint16_t)FR_DATA[0]<<8)|FR_DATA[1]); Build_OK_Reply(0x0301); break;
    case 0x0261: if(FR_LEN>=1) Channel_AutoSample_SetInterval(FR_DATA[0]); Build_OK_Reply(cmd); break;
    case 0x0302: Channel_AutoSample_Start(FR_LEN>=1 ? FR_DATA[0] : 5); Build_OK_Reply(0x0302); break;
    case 0x0303: Channel_AutoSample_Stop(); Build_OK_Reply(0x0303); break;
    case 0x03AA: Build_OK_Reply(0x03AA); delay_1ms(50); RTC_WakeupTimer_Start(10); pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD); System_Init(); USART1_SendString("instrument wakeup"); break;

    /* 参数配置 */
    case 0x0400: Build_ThresholdReply(cmd, g_param.ch0_threshold, g_param.ch1_threshold); break;
    case 0x0401: Build_DataReply(0x0401, g_param.ch0_threshold); break;
    case 0x0402: Build_DataReply(0x0402, g_param.ch1_threshold); break;
    case 0x0411: if(FR_LEN>=4){g_param.ch0_threshold=parse_float_be(FR_DATA);Param_Save();} Build_OK_Reply(0x0411); break;
    case 0x0412: if(FR_LEN>=4){g_param.ch1_threshold=parse_float_be(FR_DATA);Param_Save();} Build_OK_Reply(0x0412); break;
    case 0x0421: Build_ThresholdReply(cmd, g_param.ch0_threshold, g_param.ch1_threshold); break;

    /* OTA */
    case 0x0501: g_param.upgrade_flag=0xA5;Param_Save();Build_OK_Reply(0x0501);delay_1ms(50);NVIC_SystemReset(); break;

    /* 告警 */
    case 0x0601: if(FR_LEN>=1)Alarm_SetMode(FR_DATA[0]);Build_OK_Reply(0x0601);break;
    case 0x0602: Alarm_Query(); break;
    case 0x0603: Alarm_Clear();Build_OK_Reply(0x0603);break;

    default: Build_ErrorReply(); break;
    }
}
