#include "sys_init.h"
#include "frame_builder.h"

void System_Init(void) {
    systick_config();                    /* 1ms基准 */
    LED_Init();                          /* PA4~PA7 */
    OLED_Init();
    OLED_ShowLine1((uint8_t*)"2026CIMC");
    OLED_ShowLine2((uint8_t*)"IDLE");
    OLED_Refresh();
    USART1_Config();                     /* USART1, 19200, RS485 */
    Param_Load();                        /* Flash参数区→RAM */
    ADC_Init();
    DAC_Init();
    RTC_Init();
    delay_1ms(100);
    Build_Heartbeat();                   /* 上电心跳 0x05 0x8888 */
}
