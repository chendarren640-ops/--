/**
 * Bootloader — 收二进制/发 ASCII hex 协议层
 *
 * 评测工具: 发 → 二进制帧 | 收 ← ASCII hex 字符串
 */
#include "proto_bl.h"
#include "usart1_bl.h"

static const char hex_tab[] = "0123456789ABCDEF";

/* ================================================================
 * CRC16-Modbus
 * ================================================================ */
uint16_t BL_CRC16(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (crc & 1 ? 0xA001 : 0);
    }
    return crc;
}

/* ================================================================
 * 发送 ASCII hex 帧 (二进制帧 → ASCII hex 字符串 → USART1 发出)
 * ================================================================ */
void BL_SendHexFrame(uint8_t *bytes, uint16_t len)
{
    RS485_TX();
    for (uint16_t i = 0; i < len; i++) {
        USART1_SendByte(hex_tab[(bytes[i] >> 4) & 0x0F]);
        USART1_SendByte(hex_tab[bytes[i] & 0x0F]);
    }
    while (RESET == usart_flag_get(USART1, USART_FLAG_TC));
    RS485_RX();
}

void BL_SendOK(uint16_t cmd, uint16_t dev_id)
{
    uint8_t frm[16];
    frm[0] = 0xA5; frm[1] = 0xB6;
    frm[2] = (dev_id >> 8) & 0xFF; frm[3] = dev_id & 0xFF;
    frm[4] = 0x02;
    frm[5] = (cmd >> 8) & 0xFF; frm[6] = cmd & 0xFF;
    frm[7] = 1; frm[8] = 0x02; frm[9] = 0xFF;
    uint16_t crc = BL_CRC16(frm, 10);
    frm[10] = (crc >> 8) & 0xFF; frm[11] = crc & 0xFF;
    frm[12] = 0xB6; frm[13] = 0xA5;
    BL_SendHexFrame(frm, 14);
}

void BL_SendError(uint16_t dev_id)
{
    uint8_t frm[13];
    frm[0] = 0xA5; frm[1] = 0xB6;
    frm[2] = (dev_id >> 8) & 0xFF; frm[3] = dev_id & 0xFF;
    frm[4] = 0xFF; frm[5] = 0xEE; frm[6] = 0xEE;
    frm[7] = 0; frm[8] = 0x02;
    uint16_t crc = BL_CRC16(frm, 9);
    frm[9] = (crc >> 8) & 0xFF; frm[10] = crc & 0xFF;
    frm[11] = 0xB6; frm[12] = 0xA5;
    BL_SendHexFrame(frm, 13);
}

void BL_SendHeartbeat(uint16_t dev_id)
{
    uint8_t frm[13];
    frm[0] = 0xA5; frm[1] = 0xB6;
    frm[2] = (dev_id >> 8) & 0xFF; frm[3] = dev_id & 0xFF;
    frm[4] = 0x05; frm[5] = 0x88; frm[6] = 0x88;
    frm[7] = 0; frm[8] = 0x02;
    uint16_t crc = BL_CRC16(frm, 9);
    frm[9] = (crc >> 8) & 0xFF; frm[10] = crc & 0xFF;
    frm[11] = 0xB6; frm[12] = 0xA5;
    BL_SendHexFrame(frm, 13);
}

void BL_SendString(char *str)
{
    USART1_SendString(str);
}

/* ================================================================
 * 接收二进制帧 (直接收原始字节)
 * out_buf[0..1]=A5B6, out_buf[2..3]=DDDD, ...
 *
 * 帧结构: A5B6 | ID(2) | TT(1) | CCCC(2) | LL(1) | VER(1) | 数据(LL) | CRC(2) | B6A5
 * 总长 = 13 + LL, 最大 253 字节
 * ================================================================ */
uint8_t BL_ParseHexFrame(uint8_t *out_buf, uint16_t buf_size)
{
    uint8_t  sync = 0;
    uint16_t idx  = 0;
    uint32_t deadline;

    /* 等帧头 A5 B6 (超时 500ms) */
    deadline = bl_tick + 500;
    while (bl_tick < deadline) {
        if (USART1_Available() == 0) { delay_1ms(5); continue; }
        uint8_t b = USART1_ReadByte();
        if (sync == 0) {
            if (b == 0xA5) { out_buf[0] = 0xA5; idx = 1; sync = 1; }
        } else {
            if (b == 0xB6) { out_buf[1] = 0xB6; idx = 2; break; }
            else if (b == 0xA5) { idx = 1; }
            else { sync = 0; idx = 0; }
        }
    }
    if (sync == 0 || idx < 2) return FRAME_NEED_MORE;

    /* 收帧: 字节间超时 200ms */
    while (1) {
        deadline = bl_tick + 200;
        while (USART1_Available() == 0) {
            if (bl_tick >= deadline) return FRAME_ERROR;
            delay_1ms(5);
        }
        if (idx >= buf_size - 1) return FRAME_ERROR;
        out_buf[idx++] = USART1_ReadByte();

        /* 用长度字段精确计算帧尾位置, 只在计算位置校验 B6A5 */
        if (idx >= 9) {
            uint8_t pay_len = out_buf[7];
            if (pay_len <= 240) {
                uint16_t exp = (uint16_t)(13 + pay_len);
                if (exp <= buf_size && idx >= exp &&
                    out_buf[exp - 2] == 0xB6 && out_buf[exp - 1] == 0xA5) {
                    /* 截断到精确长度 */
                    if (idx > exp) {
                        /* 超长数据: 调 idx, 丢弃多余字节 (含 B6A5) */
                        /* 不, 这是正常的精确匹配 */
                    }
                    idx = exp;
                    break;
                }
            }
        }

        /* 回退: 简单尾标检测 (兼容不含长度字段或长度不可靠的帧) */
        if (idx >= 2 && out_buf[idx - 2] == 0xB6 && out_buf[idx - 1] == 0xA5)
            break;
    }

    /* CRC 校验 */
    if (idx < 13) return FRAME_ERROR;
    uint16_t calc_crc = BL_CRC16(out_buf, idx - 4);
    uint16_t rcv_crc  = ((uint16_t)out_buf[idx - 4] << 8) | out_buf[idx - 3];
    if (calc_crc != rcv_crc) return FRAME_ERROR;

    return FRAME_OK;
}
