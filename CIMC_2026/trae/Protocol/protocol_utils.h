/**
 * @file    protocol_utils.h
 * @brief   通信协议层工具头文件
 *
 * 核心功能:
 *   1. Hex -> ASCII 字符串转换 (赛题核心要求)
 *   2. CRC16 校验
 *   3. 帧解析与组帧
 */

#ifndef __PROTOCOL_UTILS_H
#define __PROTOCOL_UTILS_H

#include "HeaderFiles.h"

/* 帧边界定义 */
#define FRAME_HEAD_LOW   0xA5
#define FRAME_HEAD_HIGH  0xB6
#define FRAME_TAIL_LOW   0xB6
#define FRAME_TAIL_HIGH  0xA5

/* 帧类型 */
#define FRAME_TYPE_QUERY     0x01
#define FRAME_TYPE_RESP      0x02
#define FRAME_TYPE_HEART     0x05
#define FRAME_TYPE_ERROR     0xFF

/* 函数声明 */
uint8_t calc_crc16(uint8_t *data, uint16_t len);
void    byte_to_hex_str(uint8_t byte, char *hex_str);
void    hex_to_str(uint8_t *hex_data, uint16_t hex_len, char *str_out);
uint8_t hex_char_to_byte(char c);
uint8_t str_to_hex(const char *str, uint8_t *hex_out, uint16_t max_len);

#endif
