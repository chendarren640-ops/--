/**
 * 2026 CIMC APP — ASCII 十六进制协议转换工具
 *
 * 赛题核心规约:
 *   所有协议帧按十六进制结构组帧后, 以 ASCII 字符串形式收发!
 *
 *   例如: 帧头 0xA5B6 → 发送字符 'A','5','B','6' (0x41 0x35 0x42 0x36)
 *        而不是直接发送 0xA5 0xB6 两个字节。
 *
 *   部分功能 (告警查询/Bootloader提示/睡眠唤醒) 使用纯字符串直接回复,
 *   不经帧封装。
 */

#include "ascii_proto.h"

/* 十六进制字符表 */
static const char hex_chars[] = "0123456789ABCDEF";

/**
 * @brief  单字节 → 2个ASCII十六进制字符
 *         例: 0xA5 → "A5"
 */
void ByteToHexStr(uint8_t byte, char *hex_out)
{
    hex_out[0] = hex_chars[(byte >> 4) & 0x0F];
    hex_out[1] = hex_chars[byte & 0x0F];
}

/**
 * @brief  字节数组 → ASCII十六进制字符串
 *         例: {0xA5, 0xB6} → "A5B6"
 *
 * @param  bytes    输入字节数组
 * @param  len      字节长度
 * @param  hex_str  输出字符串缓冲区 (至少需要 len*2 + 1 字节)
 */
void BytesToHexStr(uint8_t *bytes, uint16_t len, char *hex_str)
{
    for (uint16_t i = 0; i < len; i++)
    {
        ByteToHexStr(bytes[i], &hex_str[i * 2]);
    }
    hex_str[len * 2] = '\0';
}

/**
 * @brief  2个ASCII十六进制字符 → 单字节
 *         例: 'A','5' → 0xA5
 */
uint8_t HexStrToByte(char high, char low)
{
    uint8_t val = 0;

    if (high >= '0' && high <= '9')       val  = (high - '0') << 4;
    else if (high >= 'A' && high <= 'F')  val  = (high - 'A' + 10) << 4;
    else if (high >= 'a' && high <= 'f')  val  = (high - 'a' + 10) << 4;

    if (low >= '0' && low <= '9')         val |= (low - '0');
    else if (low >= 'A' && low <= 'F')    val |= (low - 'A' + 10);
    else if (low >= 'a' && low <= 'f')    val |= (low - 'a' + 10);

    return val;
}

/**
 * @brief  ASCII十六进制字符串 → 字节数组
 *
 * @param  hex_str  输入 ASCII 十六进制字符串 (长度必须是偶数)
 * @param  str_len  字符串长度
 * @param  bytes    输出字节数组
 * @return 转换的字节数 (str_len / 2)
 */
uint16_t HexStrToBytes(char *hex_str, uint16_t str_len, uint8_t *bytes)
{
    uint16_t byte_cnt = str_len / 2;
    for (uint16_t i = 0; i < byte_cnt; i++)
    {
        bytes[i] = HexStrToByte(hex_str[i * 2], hex_str[i * 2 + 1]);
    }
    return byte_cnt;
}

/**
 * @brief  通过串口发送十六进制帧 (自动转 ASCII)
 *
 * @param  frame_bytes  帧字节数组
 * @param  len          字节长度
 */
void SendHexFrame(uint8_t *frame_bytes, uint16_t len)
{
    /* 直接发送二进制字节 — 上位机/评测工具使用二进制协议帧 */
    ProtoSendBytes(frame_bytes, len);
}

/**
 * @brief  检查字符是否为有效的十六进制字符 [0-9A-Fa-f]
 */
uint8_t IsHexChar(char c)
{
    return ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f'));
}
