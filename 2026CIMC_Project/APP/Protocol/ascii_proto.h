#ifndef __ASCII_PROTO_H
#define __ASCII_PROTO_H
#include "main.h"
#include "usart1_drv.h"
void     ByteToHexStr(uint8_t byte, char *hex_out);
void     BytesToHexStr(uint8_t *bytes, uint16_t len, char *hex_str);
uint8_t  HexStrToByte(char high, char low);
uint16_t HexStrToBytes(char *hex_str, uint16_t str_len, uint8_t *bytes);
void     SendHexFrame(uint8_t *frame_bytes, uint16_t len);
uint8_t  IsHexChar(char c);
#endif
