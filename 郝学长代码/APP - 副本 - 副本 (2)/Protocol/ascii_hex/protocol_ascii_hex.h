#ifndef __PROTOCOL_ASCII_HEX_H__
#define __PROTOCOL_ASCII_HEX_H__

#include <stdint.h>

int ascii_hex_to_bytes(const uint8_t *ascii, uint16_t ascii_len, uint8_t *out, uint16_t out_max);
int bytes_to_ascii_hex(const uint8_t *data, uint16_t len, uint8_t *out, uint16_t out_max);

#endif

