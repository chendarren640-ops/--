#include "crc16.h"
uint16_t CRC16_Modbus(uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for(uint16_t i=0;i<len;i++){
        crc ^= data[i];
        for(uint8_t j=0;j<8;j++) crc = (crc>>1)^(crc&1?0xA001:0);
    }
    return crc;
}
