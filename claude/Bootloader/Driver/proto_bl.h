/**
 * Bootloader — 二进制协议层 (CRC16 + 帧构建/解析)
 *
 * 评测工具直接收发二进制帧, 帧格式:
 *   A5B6 | DDDD | TT | CCCC | LL | 02 | 内容 | XXXX | B6A5
 */
#ifndef __PROTO_BL_H
#define __PROTO_BL_H
#include "../bootloader.h"

/* 帧解析结果 */
#define FRAME_OK        0
#define FRAME_NEED_MORE 1
#define FRAME_ERROR     2

/* 从 USART1 解析一帧 (二进制, 含 A5B6 前缀)
 * 返回: FRAME_OK=成功, FRAME_NEED_MORE=超时无帧头, FRAME_ERROR=格式/CRC错误
 */
uint8_t  BL_ParseHexFrame(uint8_t *out_buf, uint16_t buf_size);

/* 构建并发送帧 (二进制直发, RS-485 方向控制由 USART1_SendBytes 负责) */
void     BL_SendHexFrame(uint8_t *bytes, uint16_t len);
void     BL_SendOK(uint16_t cmd, uint16_t dev_id);
void     BL_SendError(uint16_t dev_id);
void     BL_SendHeartbeat(uint16_t dev_id);
void     BL_SendString(char *str);

/* CRC16-Modbus */
uint16_t BL_CRC16(uint8_t *data, uint16_t len);

#endif
