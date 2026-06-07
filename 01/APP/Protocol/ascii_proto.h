/**
 * 2026 CIMC APP — ASCII 协议转换工具头文件
 */

#ifndef __ASCII_PROTO_H
#define __ASCII_PROTO_H

#include "main.h"
#include "usart_drv.h"
#include "../Driver/USART/usart0_dbg.h"

/* ================================================================
 * 协议通道选择 — 只改这一个宏即可切换 CH340 ↔ RS-485
 *   0 = RS-485 (USART1 + 485_CS PA1)
 *   1 = CH340  (USART0 直连 USB 口)
 * ================================================================ */
#define PROTO_VIA_CH340  1   /* 🔧 CH340 USB调试 (改回0切到RS-485) */

#if PROTO_VIA_CH340
  #define ProtoSendString(s)   USART0_DBG_SendString(s)
  #define ProtoSendBytes(b,l)  do{for(int _i=0;_i<(l);_i++) USART0_DBG_SendByte((b)[_i]);}while(0)
#else
  #define ProtoSendString(s)   USART1_SendString(s)
  #define ProtoSendBytes(b,l)  USART1_SendBytes(b,l)
#endif

/* 函数声明 */
void     ByteToHexStr(uint8_t byte, char *hex_out);
void     BytesToHexStr(uint8_t *bytes, uint16_t len, char *hex_str);
uint8_t  HexStrToByte(char high, char low);
uint16_t HexStrToBytes(char *hex_str, uint16_t str_len, uint8_t *bytes);
void     SendHexFrame(uint8_t *frame_bytes, uint16_t len);
uint8_t  IsHexChar(char c);

#endif
