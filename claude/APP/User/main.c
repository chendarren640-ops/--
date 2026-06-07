#include "main.h"
#include "../Driver/LED/led_drv.h"
#include "../Protocol/frame_parser.h"
#include "../Function/data_channel.h"

volatile uint8_t  g_sys_state = STATE_IDLE;
volatile uint8_t  g_led_toggle_flag = 0;

int main(void)
{
    SCB->VTOR = 0x08011000;  /* 向量表偏移到 APP 区 (Bootloader 占 0x08000000) */

    System_Init();

    while (1)
    {
        /* 1. 帧解析 → 命令分发 */
        frame_parser_process();

        /* 2. LED 心跳 (SysTick 控制, 500ms 周期) */
        if (g_led_toggle_flag) {
            g_led_toggle_flag = 0;
            gpio_bit_toggle(LED_SYS_PORT, LED_SYS_PIN);
        }

        /* 3. 自动上报 (非阻塞) */
        Channel_AutoSample_Process();
    }
}
