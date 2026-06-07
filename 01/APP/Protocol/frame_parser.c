/**
 * 2026 CIMC APP — 二进制帧解析器
 *
 * 评测工具直接收发二进制帧 (非 ASCII hex 字符串)：
 *   帧头 0xA5 0xB6 → 收集 → 帧尾 0xB6 0xA5 → CRC校验 → 分发
 */
#include "frame_parser.h"
#include "crc16.h"
#include "frame_builder.h"
#include "../Function/cmd_handler.h"
#include "../Driver/Flash/flash_param.h"

uint8_t  g_frame_ready = 0;
uint8_t  g_frame_buf[FRAME_BUF_SIZE];
uint16_t g_frame_len = 0;

static uint8_t  raw_buf[FRAME_BUF_SIZE];
static uint16_t raw_idx = 0;

/* 帧头同步状态: 0=找0xA5, 1=找0xB6 */
static uint8_t sync_state = 0;

void frame_parser_feed(uint8_t byte)
{
    switch (sync_state) {
    case 0: /* 找 0xA5 */
        if (byte == 0xA5) {
            raw_buf[0] = 0xA5;
            raw_idx = 1;
            sync_state = 1;
        }
        return;

    case 1: /* 找 0xB6 完成帧头 A5B6 */
        if (byte == 0xB6) {
            raw_buf[1] = 0xB6;
            raw_idx = 2;
            sync_state = 2;
        } else if (byte == 0xA5) {
            raw_buf[0] = 0xA5;
            raw_idx = 1;
            /* sync_state 保持 1 */
        } else {
            sync_state = 0;
            raw_idx = 0;
        }
        return;
    }

    /* sync_state == 2: 正在收帧, 等 0xB6 0xA5 帧尾 */
    if (raw_idx >= FRAME_BUF_SIZE - 1) {
        /* 溢出, 丢弃 */
        sync_state = 0;
        raw_idx = 0;
        return;
    }

    raw_buf[raw_idx++] = byte;

    /* 检测帧尾 B6 A5 */
    if (raw_idx >= 2 &&
        raw_buf[raw_idx - 2] == 0xB6 &&
        raw_buf[raw_idx - 1] == 0xA5) {
        /* 帧完整, 拷贝到 g_frame_buf */
        memcpy(g_frame_buf, raw_buf, raw_idx);
        g_frame_len = raw_idx;
        g_frame_ready = 1;
        sync_state = 0;
        raw_idx = 0;
    }
}

void frame_parser_process(void)
{
    if (!g_frame_ready) return;
    g_frame_ready = 0;

    /* 最小帧: 起始2+ID2+类型1+命令2+长度1+版本1+CRC2+结束2 = 13字节 */
    if (g_frame_len < 13) return;

    /* CRC16-Modbus: 从起始标志到内容末尾 (不含CRC和结束标志) */
    uint16_t calc_crc = CRC16_Modbus(g_frame_buf, g_frame_len - 4);
    uint16_t rcv_crc = (g_frame_buf[g_frame_len - 4] << 8) | g_frame_buf[g_frame_len - 3];
    if (calc_crc != rcv_crc) {
        Build_ErrorReply();   /* CRC 错误 → FF EEEE */
        return;
    }

    /* 设备 ID 校验 */
    uint16_t frame_id = (g_frame_buf[2] << 8) | g_frame_buf[3];
    if (frame_id != 0xFFFF && frame_id != g_param.device_id) {
        return;   /* 非广播且ID不匹配 → 静默丢弃 */
    }

    /* 长度合法性校验 */
    uint8_t pay_len = g_frame_buf[7];
    if (9 + pay_len + 2 + 2 != g_frame_len) {
        Build_ErrorReply();   /* 长度不匹配 → FF EEEE (K-02) */
        return;
    }

    /* 分发到命令处理器 */
    cmd_dispatch(g_frame_buf, g_frame_len);
}
