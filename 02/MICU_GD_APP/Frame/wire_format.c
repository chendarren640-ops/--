/* SPDX-License-Identifier: MIT */

/**
 * @file    wire_format.c
 * @brief   Implementation of ASCII-hex codec and CRC-16 (Modbus) engine.
 */

#include "wire_format.h"

/* ================================================================== */
/*  ASCII-HEX codec                                                   */
/* ================================================================== */

/**
 * @brief  Return numeric value (0..15) of a single hex character,
 *         or -1 if the character is not valid hex.
 */
static int HexNib_Value(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
    {
        return (int)(ch - '0');
    }
    if ((ch >= 'A') && (ch <= 'F'))
    {
        return (int)(ch - 'A' + 10);
    }
    if ((ch >= 'a') && (ch <= 'f'))
    {
        return (int)(ch - 'a' + 10);
    }
    return -1;
}

/**
 * @brief  Lookup table for converting a 4-bit nibble to uppercase hex.
 */
static const char kHexLut[16] =
{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

bool HexStr_ToBin(const char *hex, size_t hex_len, uint8_t *bin, size_t bin_cap)
{
    size_t n;
    size_t byte_cnt;

    if ((hex == NULL) || (bin == NULL))
    {
        return false;
    }
    if ((hex_len & 1U) != 0U)
    {
        return false;
    }

    byte_cnt = hex_len / 2U;
    if (byte_cnt > bin_cap)
    {
        return false;
    }

    for (n = 0U; n < byte_cnt; n++)
    {
        int hi;
        int lo;

        hi = HexNib_Value(hex[n * 2U]);
        lo = HexNib_Value(hex[n * 2U + 1U]);

        if ((hi < 0) || (lo < 0))
        {
            return false;
        }
        bin[n] = (uint8_t)(((uint8_t)hi << 4) | (uint8_t)lo);
    }

    return true;
}

bool Bin_ToHexStr(const uint8_t *bin, size_t bin_len, char *hex, size_t hex_cap)
{
    size_t n;

    if ((bin == NULL) || (hex == NULL))
    {
        return false;
    }
    if ((bin_len * 2U) > hex_cap)
    {
        return false;
    }

    for (n = 0U; n < bin_len; n++)
    {
        hex[n * 2U]     = kHexLut[(bin[n] >> 4) & 0x0FU];
        hex[n * 2U + 1U] = kHexLut[bin[n] & 0x0FU];
    }

    return true;
}

/* ================================================================== */
/*  CRC-16 (MODBUS)  --  lookup-table implementation                  */
/* ================================================================== */

/**
 * @brief  Pre-computed CRC-16 table for polynomial 0x8005 (reversed
 *         form 0xA001), initialised with initial value 0xFFFF.
 *
 * Generating expression for entry N:
 *   crc = N;
 *   for (bit = 0; bit < 8; ++bit)
 *       crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
 */
static const uint16_t kCrc16Table[256] =
{
    0x0000U, 0xC0C1U, 0xC181U, 0x0140U, 0xC301U, 0x03C0U, 0x0280U, 0xC241U,
    0xC601U, 0x06C0U, 0x0780U, 0xC741U, 0x0500U, 0xC5C1U, 0xC481U, 0x0440U,
    0xCC01U, 0x0CC0U, 0x0D80U, 0xCD41U, 0x0F00U, 0xCFC1U, 0xCE81U, 0x0E40U,
    0x0A00U, 0xCAC1U, 0xCB81U, 0x0B40U, 0xC901U, 0x09C0U, 0x0880U, 0xC841U,
    0xD801U, 0x18C0U, 0x1980U, 0xD941U, 0x1B00U, 0xDBC1U, 0xDA81U, 0x1A40U,
    0x1E00U, 0xDEC1U, 0xDF81U, 0x1F40U, 0xDD01U, 0x1DC0U, 0x1C80U, 0xDC41U,
    0x1400U, 0xD4C1U, 0xD581U, 0x1540U, 0xD701U, 0x17C0U, 0x1680U, 0xD641U,
    0xD201U, 0x12C0U, 0x1380U, 0xD341U, 0x1100U, 0xD1C1U, 0xD081U, 0x1040U,
    0xF001U, 0x30C0U, 0x3180U, 0xF141U, 0x3300U, 0xF3C1U, 0xF281U, 0x3240U,
    0x3600U, 0xF6C1U, 0xF781U, 0x3740U, 0xF501U, 0x35C0U, 0x3480U, 0xF441U,
    0x3C00U, 0xFCC1U, 0xFD81U, 0x3D40U, 0xFF01U, 0x3FC0U, 0x3E80U, 0xFE41U,
    0xFA01U, 0x3AC0U, 0x3B80U, 0xFB41U, 0x3900U, 0xF9C1U, 0xF881U, 0x3840U,
    0x2800U, 0xE8C1U, 0xE981U, 0x2940U, 0xEB01U, 0x2BC0U, 0x2A80U, 0xEA41U,
    0xEE01U, 0x2EC0U, 0x2F80U, 0xEF41U, 0x2D00U, 0xEDC1U, 0xEC81U, 0x2C40U,
    0xE401U, 0x24C0U, 0x2580U, 0xE541U, 0x2700U, 0xE7C1U, 0xE681U, 0x2640U,
    0x2200U, 0xE2C1U, 0xE381U, 0x2340U, 0xE101U, 0x21C0U, 0x2080U, 0xE041U,
    0xA001U, 0x60C0U, 0x6180U, 0xA141U, 0x6300U, 0xA3C1U, 0xA281U, 0x6240U,
    0x6600U, 0xA6C1U, 0xA781U, 0x6740U, 0xA501U, 0x65C0U, 0x6480U, 0xA441U,
    0x6C00U, 0xACC1U, 0xAD81U, 0x6D40U, 0xAF01U, 0x6FC0U, 0x6E80U, 0xAE41U,
    0xAA01U, 0x6AC0U, 0x6B80U, 0xAB41U, 0x6900U, 0xA9C1U, 0xA881U, 0x6840U,
    0x7800U, 0xB8C1U, 0xB981U, 0x7940U, 0xBB01U, 0x7BC0U, 0x7A80U, 0xBA41U,
    0xBE01U, 0x7EC0U, 0x7F80U, 0xBF41U, 0x7D00U, 0xBDC1U, 0xBC81U, 0x7C40U,
    0xB401U, 0x74C0U, 0x7580U, 0xB541U, 0x7700U, 0xB7C1U, 0xB681U, 0x7640U,
    0x7200U, 0xB2C1U, 0xB381U, 0x7340U, 0xB101U, 0x71C0U, 0x7080U, 0xB041U,
    0x5000U, 0x90C1U, 0x9181U, 0x5140U, 0x9301U, 0x53C0U, 0x5280U, 0x9241U,
    0x9601U, 0x56C0U, 0x5780U, 0x9741U, 0x5500U, 0x95C1U, 0x9481U, 0x5440U,
    0x9C01U, 0x5CC0U, 0x5D80U, 0x9D41U, 0x5F00U, 0x9FC1U, 0x9E81U, 0x5E40U,
    0x5A00U, 0x9AC1U, 0x9B81U, 0x5B40U, 0x9901U, 0x59C0U, 0x5880U, 0x9841U,
    0x8801U, 0x48C0U, 0x4980U, 0x8941U, 0x4B00U, 0x8BC1U, 0x8A81U, 0x4A40U,
    0x4E00U, 0x8EC1U, 0x8F81U, 0x4F40U, 0x8D01U, 0x4DC0U, 0x4C80U, 0x8C41U,
    0x4400U, 0x84C1U, 0x8581U, 0x4540U, 0x8701U, 0x47C0U, 0x4680U, 0x8641U,
    0x8201U, 0x42C0U, 0x4380U, 0x8341U, 0x4100U, 0x81C1U, 0x8081U, 0x4040U
};

uint16_t Crc16_Compute(const uint8_t *data, size_t len)
{
    uint16_t crc;
    size_t   n;

    if (data == NULL)
    {
        return 0U;
    }

    crc = 0xFFFFU;
    for (n = 0U; n < len; n++)
    {
        uint8_t idx;

        idx = (uint8_t)((crc ^ (uint16_t)data[n]) & 0xFFU);
        crc = (uint16_t)((crc >> 8) ^ kCrc16Table[idx]);
    }

    return crc;
}
