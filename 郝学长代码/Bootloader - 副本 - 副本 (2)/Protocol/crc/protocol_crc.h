#ifndef __PROTOCOL_CRC_H
#define __PROTOCOL_CRC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t Protocol_CRC16_Modbus(const uint8_t *data, uint16_t len);
uint32_t Protocol_CRC32(const uint8_t *data, uint32_t len);
uint32_t Protocol_CRC32_Update(uint32_t crc, const uint8_t *data, uint32_t len);
uint32_t Protocol_CRC32_Finish(uint32_t crc);

#ifdef __cplusplus
}
#endif

#endif

