#include "protocol_ascii_hex.h"

static int hex_char_to_value(uint8_t c)
{
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

int ascii_hex_to_bytes(const uint8_t *ascii, uint16_t ascii_len, uint8_t *out, uint16_t out_max)
{
    uint16_t i;
    uint16_t out_len = 0;

    if((ascii_len % 2) != 0)
    {
        return -1;
    }

    for(i = 0; i < ascii_len; i += 2)
    {
        int h = hex_char_to_value(ascii[i]);
        int l = hex_char_to_value(ascii[i + 1]);

        if(h < 0 || l < 0)
        {
            return -2;
        }

        if(out_len >= out_max)
        {
            return -3;
        }

        out[out_len++] = (uint8_t)((h << 4) | l);
    }

    return out_len;
}

int bytes_to_ascii_hex(const uint8_t *data, uint16_t len, uint8_t *out, uint16_t out_max)
{
    static const char hex[] = "0123456789ABCDEF";
    uint16_t i;

    if(out_max < len * 2)
    {
        return -1;
    }

    for(i = 0; i < len; i++)
    {
        out[i * 2] = hex[(data[i] >> 4) & 0x0F];
        out[i * 2 + 1] = hex[data[i] & 0x0F];
    }

    return len * 2;
}

