#include "main.h"
#include "../Driver/LED/led_drv.h"
#include "../Protocol/frame_parser.h"
#include "../Function/data_channel.h"

volatile uint8_t  g_sys_state = STATE_IDLE;
volatile uint8_t  g_led_toggle_flag = 0;

int main(void)
{
    // SCB->VTOR = 0x08011000;  /* 调试模式: 独立运行, 不依赖Bootloader */

    System_Init();

    /* 诊断: 上电后先快闪 3 次 LED (不依赖 SysTick, 低电平有效) */
    for (int i = 0; i < 3; i++) {
        gpio_bit_reset(LED_SYS_PORT, LED_SYS_PIN);   /* 低电平 = 亮 */
        for (volatile uint32_t d = 0; d < 2000000; d++);
        gpio_bit_set(LED_SYS_PORT, LED_SYS_PIN);     /* 高电平 = 灭 */
        for (volatile uint32_t d = 0; d < 2000000; d++);
    }

    while (1)
    {
        /* 1. 帧解析 → 命令分发 (CRC校验 + 业务处理) */
        frame_parser_process();

        /* 2. LED 心跳 (SysTick 控制, 500ms 周期) */
        if (g_led_toggle_flag) {
            g_led_toggle_flag = 0;
            gpio_bit_toggle(LED_SYS_PORT, LED_SYS_PIN);
        }

        /* 3. 自动上报 (非阻塞, 基于 SysTick 计时) */
        Channel_AutoSample_Process();
    }
}
