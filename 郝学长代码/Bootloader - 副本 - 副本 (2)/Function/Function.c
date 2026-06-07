/*
 * Function.c
 * Bootloader 业务逻辑 - 主循环
 *
 * 正常启动: 显示 "Bootloader", 发送心跳, 等待5秒, 跳转APP
 * 升级模式: 显示 "Bootloader", 打印倒计时, 等待10秒接收固件
 */

#include "HeaderFiles.h"
#include "Function.h"
#include "bsp_usart_rs485.h"
#include "boot_flag.h"
#include "boot_jump.h"
#include "boot_update.h"
#include "boot_protocol.h"
#include "protocol_crc.h"
#include "protocol_ascii_hex.h"
#include <string.h>

/* 赛题队伍编号  */
#define TEAM_ID_STR  "2026466695"

/* 升级等待超时 */
#define BOOT_UPGRADE_WAIT_MS    10000U
#define BOOT_NORMAL_WAIT_MS     5000U
#define BOOT_OLED_REFRESH_MS    500U

/* 接收缓冲区 */
static uint8_t rx_buf[2048];
static uint16_t rx_len = 0;

/* 协议帧结构 */
static boot_frame_t frame;

/* 状态 */
static uint8_t s_is_upgrade_mode = 0;
static uint8_t s_update_started = 0;
static uint32_t s_fw_offset = 0;

extern volatile uint32_t g_boot_tick_ms;

/* OLED 显示: 第一行队伍编号, 第二行状态信息 (24px字体) */
static void Boot_OLED_Show(const char *line2)
{
    OLED_Clear();
    OLED_ShowString(0, 4,  (uint8_t *)TEAM_ID_STR, 16);
    if(line2 != NULL)
        OLED_ShowString(0, 36, (uint8_t *)line2, 16);
    OLED_Refresh();
}

/* ========== 发送心跳帧 (赛题格式) ========== */
static void Boot_SendHeartbeat(void)
{
    uint8_t frame_bin[64];
    uint8_t frame_ascii[128];
    uint16_t idx = 0;
    uint16_t crc;
    uint16_t ascii_len;

    frame_bin[idx++] = 0xA5; frame_bin[idx++] = 0xB6;   /* 帧头 */
    frame_bin[idx++] = 0x00; frame_bin[idx++] = 0x01;   /* 设备ID=0x0001 */
    frame_bin[idx++] = 0x05;                              /* 帧类型: 心跳 */
    frame_bin[idx++] = 0x88; frame_bin[idx++] = 0x88;   /* 命令字: 0x8888 */
    frame_bin[idx++] = 0x00;                              /* 内容长度 */
    frame_bin[idx++] = 0x02;                              /* 协议版本 */

    crc = Protocol_CRC16_Modbus(frame_bin, idx);
    frame_bin[idx++] = (uint8_t)(crc >> 8);
    frame_bin[idx++] = (uint8_t)(crc & 0xFF);
    frame_bin[idx++] = 0xB6; frame_bin[idx++] = 0xA5;   /* 帧尾 */

    if(Protocol_BytesToAsciiHex(frame_bin, idx, frame_ascii, &ascii_len))
        BSP_RS485_SendBuffer(frame_ascii, ascii_len);
}

/* ========== 打印倒计时信息 ========== */
static void Boot_PrintCountdown(uint32_t remaining_s)
{
    char msg[64];
    uint16_t len;
    len = snprintf(msg, sizeof(msg), "wait for start Application(%lus)......\r\n", remaining_s);
    BSP_RS485_SendBuffer((const uint8_t *)msg, len);
}

/* ========== 初始化 ========== */
void System_Init(void)
{
    systick_config();
    delay_1ms(100);

    OLED_Init();
    Boot_OLED_Show("Bootloader");

    BSP_RS485_Init(19200);

    /* 发送心跳 */
    Boot_SendHeartbeat();
}

/* ========== 处理接收到的帧 ========== */
static int Boot_HandleFrame(void)
{
    int ret;

    ret = boot_protocol_decode(rx_buf, rx_len, &frame);
    if(ret != 0)
    {
        /* 解码失败 - 可能是原始固件数据 (以 0x5A 0xA5 开头) */
        if(s_update_started && rx_len >= 256)
        {
            /* 当作原始固件数据写入 */
            if(BootUpdate_WriteChunk(rx_buf, rx_len))
            {
                s_fw_offset += rx_len;
            }
        }
        rx_len = 0;
        return 0;
    }

    rx_len = 0;

    /* 处理帧 */
    switch(frame.cmd)
    {
        case 0x8888: /* 心跳 - 忽略 */
        case 0xFFFF: /* 广播寻找 - 已通过心跳回复 */
            return 0;

        case BOOT_CMD_QUERY_BOOT_INFO:
        {
            /* 查询Bootloader信息 */
            uint8_t info[64];
            uint16_t info_len = 0;
            info[info_len++] = 0x00; /* OK */
            memcpy(&info[info_len], TEAM_ID_STR, strlen(TEAM_ID_STR));
            info_len += strlen(TEAM_ID_STR);
            BootProtocol_SendData(frame.id, frame.cmd, info, info_len);
            return 0;
        }

        case BOOT_CMD_ENTER_UPDATE:
        {
            BootFlag_SetEnterBootFlag();
            BootProtocol_SendAck(frame.id, frame.cmd, 0x00);
            s_is_upgrade_mode = 1;
            return 1;
        }

        case BOOT_CMD_UPDATE_START:
        {
            /* 准备传输固件数据包 */
            if(!BootUpdate_Start(0, 0, 0))
            {
                BootProtocol_SendAck(frame.id, frame.cmd, 0x04);
                return 0;
            }
            s_update_started = 1;
            s_fw_offset = 0;
            BootProtocol_SendAck(frame.id, frame.cmd, 0x00);
            Boot_OLED_Show("Updating");
            return 1;
        }

        case BOOT_CMD_UPDATE_END:
        {
            if(BootUpdate_End())
            {
                BootProtocol_SendAck(frame.id, frame.cmd, 0x00);
                Boot_OLED_Show("Update OK");
            }
            else
            {
                BootProtocol_SendAck(frame.id, frame.cmd, 0x06);
                Boot_OLED_Show("Update FAIL");
            }
            s_update_started = 0;
            return 0;
        }

        default:
        {
            BootProtocol_SendAck(frame.id, frame.cmd, 0x03);
            return 0;
        }
    }
}

/* ========== 主循环 ========== */
void UsrFunction(void)
{
    uint32_t start_tick;
    uint32_t last_oled_tick = 0;
    uint32_t wait_time;
    uint32_t countdown_val;
    uint32_t prev_countdown = 0xFFFFFFFF;

    /* 判断是否需要升级模式 */
    s_is_upgrade_mode = 0;
    if(BootFlag_NeedUpdate() || !BootJump_IsAppValid())
    {
        s_is_upgrade_mode = 1;
        BootFlag_ClearUpdateFlag();
    }

    /* 正常模式: 不做任何输出, 等5秒跳转 */
    /* 升级模式: 打印倒计时信息, 等10秒 */
    if(!s_is_upgrade_mode)
    {
        /* 正常模式 - 等待5秒 */
        Boot_OLED_Show("Bootloader");
        start_tick = g_boot_tick_ms;

        while((g_boot_tick_ms - start_tick) < BOOT_NORMAL_WAIT_MS)
        {
            if(BSP_RS485_GetFrame(rx_buf, &rx_len))
            {
                if(rx_len > 0)
                {
                    /* 收到任何帧 - 切换到升级模式 */
                    Boot_HandleFrame();
                    if(s_is_upgrade_mode)
                    {
                        goto upgrade_loop;
                    }
                }
            }
            delay_1ms(1);
        }

        /* 超时, 跳转APP */
        Boot_OLED_Show("IDLE");
        delay_1ms(100);
        BootJump_JumpToApp();
        return;
    }

upgrade_loop:
    /* 升级模式 - 打印倒计时, 等待10秒 */
    Boot_OLED_Show("Bootloader");
    /* 赛题要求格式: 无换行符, 直接输出 */
    BSP_RS485_SendBuffer((const uint8_t *)"using command to interrupt start Application", 46);

    start_tick = g_boot_tick_ms;
    last_oled_tick = start_tick;
    prev_countdown = 0xFFFFFFFF;

    wait_time = (s_update_started) ? 30000U : BOOT_UPGRADE_WAIT_MS;

    while(1)
    {
        uint32_t elapsed = g_boot_tick_ms - start_tick;
        uint32_t remaining_ms;

        if(elapsed >= wait_time) break;
        remaining_ms = wait_time - elapsed;

        /* 打印倒计时 */
        countdown_val = (remaining_ms + 999) / 1000;
        if(countdown_val != prev_countdown &&
           (countdown_val == 7 || countdown_val == 4 || countdown_val == 1))
        {
            prev_countdown = countdown_val;
            Boot_PrintCountdown(countdown_val);
        }

        /* 检查接收 */
        if(BSP_RS485_GetFrame(rx_buf, &rx_len))
        {
            if(rx_len > 0)
            {
                Boot_HandleFrame();
                if(s_update_started)
                {
                    /* 收到升级指令, 重置等待时间 */
                    start_tick = g_boot_tick_ms;
                    wait_time = 30000U;
                    prev_countdown = 0xFFFFFFFF;
                }
            }
        }

        /* OLED 刷新 */
        if((g_boot_tick_ms - last_oled_tick) >= BOOT_OLED_REFRESH_MS)
        {
            last_oled_tick = g_boot_tick_ms;
            Boot_OLED_Show(s_update_started ? "Updating" : "Bootloader");
        }

        delay_1ms(1);
    }

    /* 超时, 跳转APP */
    Boot_OLED_Show("IDLE");
    delay_1ms(100);
    BootJump_JumpToApp();
}
