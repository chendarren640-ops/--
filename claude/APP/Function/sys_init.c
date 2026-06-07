#include "sys_init.h"
#include "../Driver/USART/usart0_dbg.h"
#include "frame_builder.h"
#include "alarm_mgr.h"

void System_Init(void) {
    systick_config();
    USART0_DBG_Init();                   /* 调试串口 CH340, printf 走这里 */
    printf("\r\n==== 2026 CIMC APP Booting ====\r\n");
    LED_Init();
    OLED_Init();
    OLED_ShowLine1((uint8_t*)"2026CIMC");
    OLED_ShowLine2((uint8_t*)"IDLE");
    OLED_Refresh();
    printf("OLED OK\r\n");
    USART1_Config();                     /* 协议串口 RS-485 */
    printf("USART1 OK\r\n");
    Param_Load();
    printf("Device ID: %04X\r\n", g_param.device_id);
    if (g_param.baud_code == 14) {
        USART1_Config_Baud(115200UL);
        printf("Baud: 115200\r\n");
    }
    ADC_Init();
    DAC_Init();
    RTC_Init();
    Alarm_Init();
    delay_1ms(100);
    Build_Heartbeat();                   /* 协议心跳通过 USART1 RS-485 发出 */
    printf("Heartbeat sent\r\n");
    printf("==== System Ready ====\r\n");
}
