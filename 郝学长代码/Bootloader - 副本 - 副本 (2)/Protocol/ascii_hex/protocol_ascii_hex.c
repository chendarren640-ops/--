#include "protocol_ascii_hex.h"

uint8_t Protocol_HexCharToValue(uint8_t ch, uint8_t *value)
{
    if(value == 0)
    {
        return 0;
    }

    if(ch >= '0' && ch <= '9')
    {
        *value = ch - '0';
        return 1;
    }

    if(ch >= 'A' && ch <= 'F')
    {
        *value = ch - 'A' + 10;
        return 1;
    }

    if(ch >= 'a' && ch <= 'f')
    {
        *value = ch - 'a' + 10;
        return 1;
    }

    return 0;
}

uint8_t Protocol_AsciiHexToBytes(const uint8_t *ascii, uint16_t ascii_len,
                                 uint8_t *bytes, uint16_t *bytes_len)
{
    uint16_t i;
    uint16_t out_len;
    uint8_t high;
    uint8_t low;

    if(ascii == 0 || bytes == 0 || bytes_len == 0)
    {
        return 0;
    }

    if((ascii_len % 2U) != 0U)
    {
        return 0;
    }

    out_len = 0;

    for(i = 0; i < ascii_len; i += 2U)
    {
        if(!Protocol_HexCharToValue(ascii[i], &high))
        {
            return 0;
        }

        if(!Protocol_HexCharToValue(ascii[i + 1U], &low))
        {
            return 0;
        }

        bytes[out_len++] = (uint8_t)((high << 4) | low);
    }

    *bytes_len = out_len;

    return 1;
}

uint8_t Protocol_BytesToAsciiHex(const uint8_t *bytes, uint16_t bytes_len,
                                 uint8_t *ascii, uint16_t *ascii_len)
{
    static const uint8_t table[] = "0123456789ABCDEF";
    uint16_t i;

    if(bytes == 0 || ascii == 0 || ascii_len == 0)
    {
        return 0;
    }

    for(i = 0; i < bytes_len; i++)
    {
        ascii[i * 2U] = table[(bytes[i] >> 4) & 0x0F];
        ascii[i * 2U + 1U] = table[bytes[i] & 0x0F];
    }

    *ascii_len = bytes_len * 2U;

    return 1;
}

