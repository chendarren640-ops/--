/*
 * boot_protocol.c
 * Bootloader 通讯协议 - 兼容赛题上位机协议格式
 * 帧格式: A5B6 [ID_H][ID_L] [Type] [Cmd_H][Cmd_L] [Len] [Ver] [Content...] [CRC_H][CRC_L] B6A5
 */

#include "boot_protocol.h"
#include "protocol_crc.h"
#include "protocol_ascii_hex.h"
#include "bsp_usart_rs485.h"
#include "boot_update.h"
#include "boot_jump.h"
#include "boot_flag.h"
#include <string.h>

static uint8_t s_tx_bin[BOOT_PROTOCOL_MAX_FRAME_LEN];
static uint8_t s_tx_ascii[BOOT_PROTOCOL_MAX_ASCII_LEN];

static uint16_t BootProtocol_ReadU16BE(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

static uint32_t BootProtocol_ReadU32BE(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

/*
 * boot_protocol_decode - 解析赛题格式的 ASCII HEX 帧
 * 帧格式 (二进制):
 *   [0] A5  [1] B6           - 帧头
 *   [2] ID_H [3] ID_L        - 设备ID (2字节)
 *   [4] Type                  - 帧类型 (0x01=命令, 0x05=心跳)
 *   [5] Cmd_H [6] Cmd_L      - 命令字 (2字节)
 *   [7] Len                   - 内容长度 (N字节)
 *   [8] Ver                   - 协议版本 (0x02)
 *   [9..9+N-1] Content        - 内容
 *   [9+N] CRC_H [10+N] CRC_L - CRC16
 *   [11+N] B6 [12+N] A5      - 帧尾
 */
int boot_protocol_decode(uint8_t *ascii, uint16_t ascii_len, boot_frame_t *frame)
{
    uint8_t bin[BOOT_PROTOCOL_MAX_FRAME_LEN];
    uint16_t bin_len;
    uint16_t content_len;
    uint16_t crc_recv;
    uint16_t crc_calc;

    if(ascii == 0 || frame == 0) return -1;

    /* ASCII HEX 转二进制 */
    if(!Protocol_AsciiHexToBytes(ascii, ascii_len, bin, &bin_len))
        return -1;

    /* 最小帧长度: 2+2+1+2+1+1+2+2 = 13 字节 */
    if(bin_len < 13U) return -1;

    /* 帧头检查 */
    if(bin[0] != 0xA5 || bin[1] != 0xB6) return -1;

    /* 帧尾检查 */
    if(bin[bin_len-2] != 0xB6 || bin[bin_len-1] != 0xA5) return -1;

    /* 内容长度 */
    content_len = bin[7];

    /* 总长度校验: 9(固定头) + content_len + 4(CRC+尾) */
    if((uint16_t)(9 + content_len + 4) != bin_len) return -1;

    /* CRC 校验: 对 [帧头~内容] 即 bin[0..8+content_len] */
    crc_calc = Protocol_CRC16_Modbus(bin, (uint16_t)(9 + content_len));
    crc_recv = ((uint16_t)bin[9 + content_len] << 8) | bin[10 + content_len];
    if(crc_recv != crc_calc) return -2;

    /* 帧类型检查: 只接受命令帧 (0x01) 和心跳帧 (0x05) */
    if(bin[4] != 0x01 && bin[4] != 0x05) return -3;

    /* 设备ID检查: 广播 0xFFFF 或匹配 */
    {
        uint16_t dev_id = ((uint16_t)bin[2] << 8) | bin[3];
        if(dev_id != 0xFFFF && dev_id != BOOT_DEVICE_ID_DEFAULT)
            return -4;
    }

    /* 提取命令字 */
    frame->id = ((uint16_t)bin[2] << 8) | bin[3]; /* 2字节设备ID */
    frame->cmd = ((uint16_t)bin[5] << 8) | bin[6];
    frame->len = content_len;

    if(content_len > 0 && content_len <= BOOT_PROTOCOL_MAX_DATA_LEN)
    {
        memcpy(frame->data, &bin[9], content_len);
    }

    return 0; /* 成功 */
}

uint8_t BootProtocol_ParseAsciiFrame(const uint8_t *ascii, uint16_t ascii_len, boot_frame_t *frame)
{
    if(boot_protocol_decode((uint8_t*)ascii, ascii_len, frame) == 0)
        return BOOT_ACK_OK;
    return BOOT_ACK_ERR_FRAME;
}

uint8_t BootProtocol_BuildBinaryFrame(uint16_t id, uint16_t cmd,
                                      const uint8_t *data, uint16_t data_len,
                                      uint8_t *out, uint16_t *out_len)
{
    uint16_t idx = 0;
    uint16_t crc;

    if(out == 0 || out_len == 0) return 0;

    /* 使用赛题格式回复 */
    out[idx++] = 0xA5; out[idx++] = 0xB6;           /* 帧头 */
    out[idx++] = (uint8_t)(id >> 8);                 /* 设备ID高字节 */
    out[idx++] = (uint8_t)(id & 0xFF);               /* 设备ID低字节 */
    out[idx++] = 0x02;                                /* 帧类型: 应答 */
    out[idx++] = (uint8_t)(cmd >> 8);                 /* 命令字高 */
    out[idx++] = (uint8_t)(cmd);                      /* 命令字低 */
    out[idx++] = (uint8_t)data_len;                   /* 内容长度 */
    out[idx++] = 0x02;                                /* 协议版本 */

    if(data_len > 0 && data != 0)
    {
        memcpy(&out[idx], data, data_len);
        idx += data_len;
    }

    /* CRC 对 [帧头~内容] */
    crc = Protocol_CRC16_Modbus(out, idx);
    out[idx++] = (uint8_t)(crc >> 8);
    out[idx++] = (uint8_t)(crc & 0xFF);

    out[idx++] = 0xB6; out[idx++] = 0xA5;           /* 帧尾 */

    *out_len = idx;
    return 1;
}

uint8_t BootProtocol_BuildAsciiFrame(uint16_t id, uint16_t cmd,
                                     const uint8_t *data, uint16_t data_len,
                                     uint8_t *out_ascii, uint16_t *out_ascii_len)
{
    uint16_t bin_len;
    if(!BootProtocol_BuildBinaryFrame(id, cmd, data, data_len, s_tx_bin, &bin_len))
        return 0;
    return Protocol_BytesToAsciiHex(s_tx_bin, bin_len, out_ascii, out_ascii_len);
}

void BootProtocol_SendAck(uint16_t id, uint16_t cmd, uint8_t status)
{
    uint8_t data[1];
    uint16_t ascii_len;
    data[0] = status;
    if(BootProtocol_BuildAsciiFrame(id, cmd, data, 1, s_tx_ascii, &ascii_len))
        BSP_RS485_SendBuffer(s_tx_ascii, ascii_len);
}

void BootProtocol_SendData(uint16_t id, uint16_t cmd, const uint8_t *data, uint16_t len)
{
    uint16_t ascii_len;
    if(BootProtocol_BuildAsciiFrame(id, cmd, data, len, s_tx_ascii, &ascii_len))
        BSP_RS485_SendBuffer(s_tx_ascii, ascii_len);
}

/*
 * BootProtocol_ProcessFrame - 处理接收到的命令帧
 * 返回值: 0=普通, 1=进入升级模式, 2=跳转APP
 */
int BootProtocol_ProcessFrameEx(const boot_frame_t *frame)
{
    uint32_t fw_size = 0;
    uint32_t fw_crc32 = 0;
    uint32_t fw_version = 0;

    if(frame == 0) return 0;

    switch(frame->cmd)
    {
        case BOOT_CMD_QUERY_BOOT_INFO:
        {
            /* 返回 Bootloader 信息 */
            uint8_t info[64];
            uint16_t info_len = 0;
            info[info_len++] = BOOT_ACK_OK;
            memcpy(&info[info_len], "Bootloader", 10);
            info_len += 10;
            BootProtocol_SendData(frame->id, frame->cmd, info, info_len);
            return 0;
        }

        case BOOT_CMD_ENTER_UPDATE:
        {
            /* APP 发来的升级请求 - 设置标志并回复 OK */
            BootFlag_SetEnterBootFlag();
            BootProtocol_SendAck(frame->id, frame->cmd, BOOT_ACK_OK);
            return 1; /* 进入升级模式 */
        }

        case BOOT_CMD_UPDATE_START:
        {
            /* 准备传输固件数据包 (0x0502) */
            /* 内容可能包含固件信息 (size, crc32, version 各4字节) */
            if(frame->len >= 12U)
            {
                fw_size = BootProtocol_ReadU32BE(&frame->data[0]);
                fw_crc32 = BootProtocol_ReadU32BE(&frame->data[4]);
                fw_version = BootProtocol_ReadU32BE(&frame->data[8]);
            }

            /* 擦除固件暂存区 */
            if(!BootUpdate_Start(fw_size, fw_crc32, fw_version))
            {
                BootProtocol_SendAck(frame->id, frame->cmd, BOOT_ACK_ERR_FLASH);
                return 0;
            }

            BootProtocol_SendAck(frame->id, frame->cmd, BOOT_ACK_OK);
            return 1; /* 进入升级模式, 等待接收固件数据 */
        }

        case BOOT_CMD_UPDATE_END:
        {
            /* 执行升级流程 (0x0503) */
            if(BootUpdate_End())
            {
                BootProtocol_SendAck(frame->id, frame->cmd, BOOT_ACK_OK);
            }
            else
            {
                BootProtocol_SendAck(frame->id, frame->cmd, BOOT_ACK_ERR_APP);
            }
            return 0;
        }

        case BOOT_CMD_JUMP_APP:
        {
            BootProtocol_SendAck(frame->id, frame->cmd, BOOT_ACK_OK);
            return 2; /* 跳转 APP */
        }

        default:
        {
            BootProtocol_SendAck(frame->id, frame->cmd, BOOT_ACK_ERR_CMD);
            return 0;
        }
    }
}

void BootProtocol_ProcessFrame(const boot_frame_t *frame)
{
    BootProtocol_ProcessFrameEx(frame);
}
