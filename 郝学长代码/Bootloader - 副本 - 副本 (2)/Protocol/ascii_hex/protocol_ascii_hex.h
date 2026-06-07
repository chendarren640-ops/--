#ifndef __PROTOCOL_ASCII_HEX_H
#define __PROTOCOL_ASCII_HEX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t Protocol_HexCharToValue(uint8_t ch, uint8_t *value);
uint8_t Protocol_AsciiHexToBytes(const uint8_t *ascii, uint16_t ascii_len,
                                 uint8_t *bytes, uint16_t *bytes_len);

uint8_t Protocol_BytesToAsciiHex(const uint8_t *bytes, uint16_t bytes_len,
                                 uint8_t *ascii, uint16_t *ascii_len);

#ifdef __cplusplus
}
#endif

#endif

