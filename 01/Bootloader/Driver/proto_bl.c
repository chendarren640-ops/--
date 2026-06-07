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
 * ================================================================ */
uint8_t BL_ParseHexFrame(uint8_t *out_buf, uint16_t buf_size)
{
    uint8_t  sync = 0;
    uint16_t idx  = 0;
    uint32_t timeout = 0;

    /* 等帧头 A5 B6 */
    while (timeout < 1000000) {
        if (USART1_Available() == 0) { timeout++; delay_1ms(1); continue; }
        timeout = 0;
        uint8_t b = USART1_ReadByte();
        if (sync == 0) {
            if (b == 0xA5) { out_buf[0] = 0xA5; idx = 1; sync = 1; }
        } else {
            if (b == 0xB6) { out_buf[1] = 0xB6; idx = 2; break; }
            else if (b == 0xA5) { idx = 1; }
            else { sync = 0; idx = 0; }
        }
    }
    if (timeout >= 1000000) return FRAME_NEED_MORE;

    /* 收帧直到 B6 A5 */
    while (1) {
        timeout = 0;
        while (USART1_Available() == 0) {
            if (++timeout > 500000) return FRAME_ERROR;
            delay_1ms(1);
        }
        if (idx >= buf_size) return FRAME_ERROR;
        out_buf[idx++] = USART1_ReadByte();
        if (idx >= 2 && out_buf[idx - 2] == 0xB6 && out_buf[idx - 1] == 0xA5)
            break;
    }

    /* CRC 校验 */
    if (idx < 6) return FRAME_ERROR;
    uint16_t calc_crc = BL_CRC16(out_buf, idx - 4);
    uint16_t rcv_crc  = ((uint16_t)out_buf[idx - 4] << 8) | out_buf[idx - 3];
    if (calc_crc != rcv_crc) return FRAME_ERROR;

    return FRAME_OK;
}
