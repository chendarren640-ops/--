/**
 * 2026 CIMC APP — 二进制帧解析器
 *
 * 评测工具直接收发二进制帧：
 *   帧头 A5B6 → 读长度字段 → 计算帧尾位置 → 校验 B6A5 → CRC → 分发
 *
 * 帧结构: A5B6 | ID(2) | TT(1) | CCCC(2) | LL(1) | VER(1) | 数据(LL) | CRC(2) | B6A5
 * 总长 = 13 + LL 字节
 */
#include "frame_parser.h"
#include "crc16.h"
#include "frame_builder.h"
#include "../Function/cmd_handler.h"
#include "../Driver/Flash/flash_param.h"

volatile uint8_t  g_frame_ready = 0;   /* volatile: ISR 写, 主循环读 */
uint8_t  g_frame_buf[FRAME_BUF_SIZE];
uint16_t g_frame_len = 0;

static uint8_t  raw_buf[FRAME_BUF_SIZE];
static uint16_t raw_idx = 0;
static uint8_t  sync_state = 0;         /* 0=找A5, 1=找B6, 2=收帧中 */
static uint32_t last_byte_tick = 0;    /* 帧内字节超时 */

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
            last_byte_tick = g_sys_tick;
        } else if (byte == 0xA5) {
            raw_buf[0] = 0xA5;
            raw_idx = 1;
        } else {
            sync_state = 0;
            raw_idx = 0;
        }
        return;
    }

    /* sync_state == 2: 正在收帧 */
    /* 帧内超时保护: 100ms 无新字节 → 丢弃残帧 */
    if ((g_sys_tick - last_byte_tick) > 100) {
        sync_state = (byte == 0xA5) ? (raw_buf[0]=0xA5, raw_idx=1, 1) : 0;
        if (sync_state == 0) raw_idx = 0;
        return;
    }
    last_byte_tick = g_sys_tick;

    /* 溢出保护: 丢弃并检查当前字节是否为下一帧帧头 */
    if (raw_idx >= FRAME_BUF_SIZE - 1) {
        sync_state = (byte == 0xA5) ? (raw_buf[0]=0xA5, raw_idx=1, 1) : 0;
        if (sync_state == 0) raw_idx = 0;
        return;
    }

    raw_buf[raw_idx++] = byte;

    /* 需要至少 9 字节头部才能读长度字段: ID(2)+TT(1)+CMD(2)+LL(1)+VER(1) = 7,
     * 加上帧头 2 字节 = 9 字节 */
    if (raw_idx < 9) return;

    uint8_t payload_len = raw_buf[7];
    if (payload_len > 240) {
        sync_state = (byte == 0xA5) ? (raw_buf[0]=0xA5, raw_idx=1, 1) : 0;
        if (sync_state == 0) raw_idx = 0;
        return;
    }

    uint16_t expected_len = 13 + payload_len;
    if (expected_len > FRAME_BUF_SIZE) {
        sync_state = 0; raw_idx = 0;
        return;
    }

    if (raw_idx < expected_len) return; /* 还没收完 */

    /* 在期望位置校验 B6A5 */
    uint16_t end = expected_len - 2;
    if (raw_buf[end] != 0xB6 || raw_buf[end + 1] != 0xA5) {
        /* 帧尾不匹配 → 丢弃, 回溯最后字节 */
        uint8_t last = raw_buf[raw_idx - 1];
        sync_state = (last == 0xA5) ? (raw_buf[0]=0xA5, raw_idx=1, 1) : 0;
        if (sync_state == 0) raw_idx = 0;
        return;
    }

    /* 帧完整: 拷贝并设就绪标志 */
    memcpy(g_frame_buf, raw_buf, expected_len);
    g_frame_len = expected_len;
    sync_state = 0;
    raw_idx = 0;
    g_frame_ready = 1;
}

void frame_parser_process(void)
{
    uint8_t ready;

    __disable_irq();
    ready = g_frame_ready;
    if (ready) {
        g_frame_ready = 0;
        /* 拷贝到栈上防止 ISR 覆盖 */
        uint8_t local_buf[FRAME_BUF_SIZE];
        uint16_t local_len = g_frame_len;
        memcpy(local_buf, g_frame_buf, g_frame_len);
        __enable_irq();

        /* 最小帧校验 */
        if (local_len < 13) return;

        /* CRC16-Modbus: 覆盖 [0 .. len-5], 不含 CRC(2) 和帧尾 B6A5(2) */
        uint16_t calc_crc = CRC16_Modbus(local_buf, local_len - 4);
        uint16_t rcv_crc = (local_buf[local_len - 4] << 8) | local_buf[local_len - 3];
        if (calc_crc != rcv_crc) {
            Build_ErrorReply();
            return;
        }

        /* 设备 ID 校验 */
        uint16_t frame_id = (local_buf[2] << 8) | local_buf[3];
        if (frame_id != 0xFFFF && frame_id != g_param.device_id) {
            return;
        }

        /* 长度字段校验 */
        uint8_t pay_len = local_buf[7];
        if ((uint16_t)(13 + pay_len) != local_len) {
            Build_ErrorReply();
            return;
        }

        cmd_dispatch(local_buf, local_len);
    } else {
        __enable_irq();
    }
}
