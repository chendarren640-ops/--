/* SPDX-License-Identifier: MIT */

/**
 * @file    wire_format.h
 * @brief   Low-level wire-format helpers: ASCII-hex conversion and
 *          CRC-16 (Modbus) integrity check.
 *
 * These utilities operate on raw byte buffers and know nothing about
 * the higher-level framing protocol.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief  Convert an ASCII-hex string to its binary representation.
 * @param  hex    Input buffer of ASCII hex characters [0-9A-Fa-f].
 * @param  hex_len Number of hex characters (must be even).
 * @param  bin    Output buffer for decoded bytes.
 * @param  bin_cap Capacity of @p bin in bytes.
 * @return true on success, false on parameter error or invalid hex digit.
 */
bool HexStr_ToBin(const char *hex, size_t hex_len, uint8_t *bin, size_t bin_cap);

/**
 * @brief  Convert a binary buffer to uppercase ASCII-hex.
 * @param  bin     Input byte buffer.
 * @param  bin_len Number of input bytes.
 * @param  hex     Output buffer for hex characters.
 * @param  hex_cap Capacity of @p hex in characters (must be >= 2 * bin_len).
 * @return true on success, false on parameter error or insufficient space.
 */
bool Bin_ToHexStr(const uint8_t *bin, size_t bin_len, char *hex, size_t hex_cap);

/**
 * @brief  Compute the Modbus CRC-16 checksum (polynomial 0x8005, reversed).
 * @param  data  Pointer to byte buffer (may be NULL only if len == 0).
 * @param  len   Number of bytes to process.
 * @return 16-bit CRC value in native byte order.
 * @note   Uses a pre-computed 256-entry lookup table for speed.
 */
uint16_t Crc16_Compute(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
