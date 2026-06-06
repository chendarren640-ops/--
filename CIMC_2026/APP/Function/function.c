/**
 * @file    function.c
 * @brief   业务逻辑层 - 系统初始化与主循环
 *
 * 2026 赛题核心功能骨架:
 *   System_Init(): 上电初始化所有外设
 *   UsrFunction(): 主循环 (LED心跳 + OLED显示 + 串口命令处理)
 */

#include "function.h"
#include "LED.h"

/* 全局变量 */
volatile uint32_t g_sys_tick_ms = 0;   /* 系统毫秒计数 */
uint32_t g_team_id = 2026001001;       /* 队伍编号, 修改为你的实际 ID */

/* 状态变量 */
uint8_t g_sys_status = SYS_STATUS_IDLE;
uint8_t g_app_running = 0;

/**
 * @brief 系统初始化
 * @note  按顺序初始化所有外设
 */
void System_Init(void)
{
    /* 1. SysTick 1ms 时基 */
    systick_config();

    /* 2. USART1 串口 (19200-8N1) - 最先初始化, 确保 printf 可用 */
    USART1_Config();
    delay_1ms(10);

    /* 3. 打印启动信息 */
    printf("\r\n==== CIMC 2026 APP Start ====\r\n");
    printf("Team ID: %lu\r\n", g_team_id);

    /* 4. LED 初始化 */
    LED_Init();

    /* 5. OLED 初始化 */
    OLED_Init();
    delay_1ms(100);

    /* 6. 显示初始状态 */
    OLED_ShowString(0, 0, (uint8_t *)"IDLE", 16);
    OLED_Refresh();

    /* 7. 打印就绪信息 */
    printf("==== System Ready ====\r\n");

    g_app_running = 1;
}

/**
 * @brief 用户主循环
 * @note  LED心跳 + OLED刷新 + 业务处理
 */
void UsrFunction(void)
{
    uint32_t last_led_tick = 0;
    uint32_t led_state = 0;

    while(1)
    {
        /* LED 1s 心跳闪烁 */
        if(g_sys_tick_ms - last_led_tick >= 500) {
            last_led_tick = g_sys_tick_ms;
            if(led_state) {
                LED_SYS_OFF();
            } else {
                LED_SYS_ON();
            }
            led_state = !led_state;
        }
    }
}
