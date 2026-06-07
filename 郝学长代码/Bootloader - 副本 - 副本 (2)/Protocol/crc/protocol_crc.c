#include "protocol_crc.h"

uint16_t Protocol_CRC16_Modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i;
    uint8_t j;

    if(data == 0)
    {
        return 0;
    }

    for(i = 0; i < len; i++)
    {
        crc ^= data[i];

        for(j = 0; j < 8; j++)
        {
            if(crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

uint32_t Protocol_CRC32_Update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint8_t j;

    if(data == 0)
    {
        return crc;
    }

    for(i = 0; i < len; i++)
    {
        crc ^= data[i];

        for(j = 0; j < 8; j++)
        {
            if(crc & 1U)
            {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

uint32_t Protocol_CRC32_Finish(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFUL;
}

uint32_t Protocol_CRC32(const uint8_t *data, uint32_t len)
{
    uint32_t crc;

    crc = 0xFFFFFFFFUL;
    crc = Protocol_CRC32_Update(crc, data, len);

    return Protocol_CRC32_Finish(crc);
}

