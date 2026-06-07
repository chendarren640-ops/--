#include "main.h"
#include "../Driver/LED/led_drv.h"
#include "../Driver/USART/usart_drv.h"
#include "../Driver/RTC/rtc_drv.h"
#include "../Protocol/frame_parser.h"
#include "../Function/data_channel.h"

volatile uint8_t  g_sys_state = STATE_IDLE;
volatile uint8_t  g_led_toggle_flag = 0;
volatile uint8_t  g_sleep_flag = 0;

int main(void)
{
    SCB->VTOR = 0x08011000;
    System_Init();

    while (1)
    {
        frame_parser_process();

        if (g_led_toggle_flag) {
            g_led_toggle_flag = 0;
            gpio_bit_toggle(LED_SYS_PORT, LED_SYS_PIN);
        }

        Channel_AutoSample_Process();

        if (g_sleep_flag) {
            g_sleep_flag = 0;
            delay_1ms(50);
            RTC_WakeupTimer_Start(10);
            pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);
            SystemInit();
            systick_config();
            USART1_Config();
            USART1_SendString("instrument wakeup\r\n");
        }
    }
}
