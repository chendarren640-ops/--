#include "sys_init.h"
#include "../Driver/USART/usart0_dbg.h"
#include "../Driver/PT100/pt100_drv.h"
#include "ascii_proto.h"
#include "frame_builder.h"
#include "alarm_mgr.h"

void System_Init(void) {
    systick_config();
    USART0_DBG_Init();                   /* 调试串口 CH340, printf 走这里 */
    printf("\r\n==== 2026 CIMC APP Booting ====\r\n");
    LED_Init();
    OLED_Init();
    OLED_ShowLine1((uint8_t*)"2026263626");
    OLED_ShowLine2((uint8_t*)"IDLE");
    OLED_Refresh();
    printf("OLED OK\r\n");
#if !PROTO_VIA_CH340
    USART1_Config();                     /* 协议串口 RS-485 (仅在 RS-485 模式) */
    printf("USART1 OK\r\n");
#else
    printf("USART1 SKIP (CH340 mode)\r\n");
#endif
    Param_Load();
#if !PROTO_VIA_CH340
    /* 根据保存的波特率重配协议 USART1 (M-02 持久化) */
    {
        uint32_t baud = (g_param.baud_code == 14) ? 115200UL : 19200UL;
        USART1_Config_Baud(baud);
    }
#endif
    printf("Device ID: %04X\r\n", g_param.device_id);
    ADC_Init();
    DAC_Init();
    RTC_Init();
    PT100_Init();                          /* PT100 采样板 I2C(PB8/PB9)+GD30AD3340 */
    Alarm_Init();
    delay_1ms(100);
    Build_Heartbeat();                   /* 协议心跳帧通过当前协议口发出 */
    printf("==== System Ready ====\r\n");
}
