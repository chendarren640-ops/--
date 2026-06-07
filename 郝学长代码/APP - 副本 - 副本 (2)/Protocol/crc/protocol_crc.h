#ifndef __PROTOCOL_CRC_H__
#define __PROTOCOL_CRC_H__

#include <stdint.h>

/**
 * @brief  CRC16-Modbus 校验计算
 * @param  data  数据缓冲区指针（从起始标志到内容末尾，不含CRC和结束标志）
 * @param  len   数据长度（字节数）
 * @return 16位CRC校验值（大端序传输时需高字节在前）
 */
uint16_t Protocol_CRC16_Modbus(const uint8_t *data, uint16_t len);

#endif /* __PROTOCOL_CRC_H__ */


