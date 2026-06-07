/*
 * Function.c
 * 业务逻辑层 - 系统初始化和主循环
 */

#include "Function.h"
#include "gd32f4xx.h"
#include "systick.h"
#include "LED.h"
#include "oled.h"
#include "bsp_adc.h"
#include "bsp_dac.h"
#include "bsp_i2c.h"
#include "gd30ad3340.h"
#include "bsp_flash.h"
#include "bsp_rtc.h"
#include "bsp_usart_rs485.h"
#include "protocol_cmd.h"
#include "app_param.h"
#include "app_sample.h"
#include "app_alarm.h"
#include "app_autoreport.h"
#include "app_sleep.h"
#include "string.h"
#include "stdio.h"
#include "protocol_ascii_hex.h"

/* 外部函数声明 */
extern uint32_t app_millis(void);

/* 全局参数 */
extern app_param_t g_app_param;

/* 静态变量 */
static uint32_t s_led_tick = 0;

#define TEAM_ID_STR "2026466695"  

/*
 * 发送心跳帧 (上电时发送一次)
 */
static void send_heartbeat(void)
{
    uint8_t frame[256];
    uint16_t idx = 0;
    uint16_t crc;

    frame[idx++] = 0xA5;
    frame[idx++] = 0xB6;
    frame[idx++] = (g_app_param.device_id >> 8) & 0xFF;
    frame[idx++] = g_app_param.device_id & 0xFF;
    frame[idx++] = 0x05;                    /* 帧类型: 心跳 */
    frame[idx++] = 0x88;
    frame[idx++] = 0x88;                    /* 命令字 0x8888 */
    frame[idx++] = 0x00;                    /* 内容长度 */
    frame[idx++] = PROTOCOL_VERSION;        /* 协议版本 */

    crc = Protocol_CRC16_Modbus(frame, idx);
    frame[idx++] = (crc >> 8) & 0xFF;
    frame[idx++] = crc & 0xFF;
    frame[idx++] = 0xB6;
    frame[idx++] = 0xA5;

    uint8_t ascii_buf[256];
    int ascii_len = bytes_to_ascii_hex(frame, idx, ascii_buf, sizeof(ascii_buf));
    if (ascii_len > 0)
    {
        BSP_RS485_SendData(ascii_buf, ascii_len);
    }
}

/*
 * 更新 OLED 显示
 */
/* OLED 显示: 第一行队伍编号, 第二行状态信息 (24px字体) */
static void update_oled(const char *state_str)
{
    OLED_Clear();
    OLED_ShowString(0, 4,  (uint8_t *)TEAM_ID_STR, 16);
    OLED_ShowString(0, 36, (uint8_t *)state_str, 16);
    OLED_Refresh();
}

/*
 * 系统初始化 (由 main 调用)
 */
void System_Init(void)
{
    /* 底层硬件初始化 */
    systick_config();
    delay_1ms(100);

    /* 外设初始化 */
    LED_Init();
    BSP_ADC_Init();
    BSP_DAC_Init();
    BSP_I2C_Init();
    GD30AD3340_Init();
    BSP_RS485_Init(19200);  /* 默认波特率 19200 */
    bsp_rtc_init();
    OLED_Init();

    /* 参数加载 */
    APP_Param_Init();

    /* 根据加载的参数重新配置波特率 */
    {
        uint32_t actual_baudrate = APP_Param_GetBaudrateValue(g_app_param.baudrate);
        if (actual_baudrate != 19200)
        {
            BSP_RS485_SetBaudrate(actual_baudrate);
        }
    }

    /* 功能模块初始化 */
    APP_Sample_Init();
    APP_Alarm_Init();
    APP_AutoReport_Init();
    APP_Sleep_Init();

    /* 上电心跳 */
    send_heartbeat();

    /* OLED 初始显示 */
    update_oled("IDLE");

    /* 系统指示灯闪一下 */
    LED_SYS_ON();
    delay_1ms(500);
    LED_SYS_OFF();

    s_led_tick = app_millis();
}

/*
 * 用户主循环 (由 main 调用，永不返回)
 */
void UsrFunction(void)
{
    while (1)
    {
        uint32_t now = app_millis();

        /* 1. 协议处理 */
        Protocol_Process();

        /* 2. 数据采样更新 */
        APP_Sample_Update();

        /* 3. 自动上报 */
        APP_AutoReport_Task();

        /* 4. 告警检测 */
        app_sample_t sample;
        APP_Sample_GetLatest(&sample);
        APP_Alarm_Check(&sample);

        /* 5. 系统指示灯闪烁 (1s 周期, 500ms 翻转) */
        if (now - s_led_tick >= 500)
        {
            s_led_tick = now;
            LED_SYS_TOGGLE();
        }

        /* 6. 工作指示灯和 OLED 更新 */
        if (g_app_param.autoreport_enable)
        {
            LED_WORK_ON();
            update_oled("AutoSample");
        }
        else
        {
            LED_WORK_OFF();
            update_oled("IDLE");
        }

        /* 7. 防止 CPU 空转 */
        delay_1ms(10);
    }
}
