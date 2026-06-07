#include "protocol_crc.h"

/**
 * @brief  CRC16-Modbus 查表法或直接计算实现（多项式 0x8005，初始值 0xFFFF）
 *         反转后多项式为 0xA001，输出不异或
 */
uint16_t Protocol_CRC16_Modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i;
    uint8_t j;

    for(i = 0; i < len; i++)
    {
        crc ^= data[i];
        for(j = 0; j < 8; j++)
        {
            if(crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}


