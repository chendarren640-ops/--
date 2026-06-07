#ifndef __BOOT_PROTOCOL_H
#define __BOOT_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TEAM_ID_STR                         "2026466695"

#define BOOT_FRAME_HEAD_H                   0xA5
#define BOOT_FRAME_HEAD_L                   0xB6
#define BOOT_FRAME_TAIL_H                   0xB6
#define BOOT_FRAME_TAIL_L                   0xA5

#define BOOT_DEVICE_ID_DEFAULT              0x0001

#define BOOT_CMD_QUERY_BOOT_INFO            0x0101
#define BOOT_CMD_ENTER_UPDATE               0x0501
#define BOOT_CMD_UPDATE_START               0x0502
#define BOOT_CMD_UPDATE_DATA                0x0504
#define BOOT_CMD_UPDATE_END                 0x0503
#define BOOT_CMD_JUMP_APP                   0x05FF

#define BOOT_ACK_OK                         0x00
#define BOOT_ACK_ERR_FRAME                  0x01
#define BOOT_ACK_ERR_CRC                    0x02
#define BOOT_ACK_ERR_CMD                    0x03
#define BOOT_ACK_ERR_FLASH                  0x04
#define BOOT_ACK_ERR_PARAM                  0x05
#define BOOT_ACK_ERR_APP                    0x06

#define BOOT_PROTOCOL_MAX_DATA_LEN          512
#define BOOT_PROTOCOL_MAX_FRAME_LEN         600
#define BOOT_PROTOCOL_MAX_ASCII_LEN         1200

typedef struct boot_frame_struct
{
    uint16_t id;
    uint16_t cmd;
    uint16_t len;
    uint8_t  data[BOOT_PROTOCOL_MAX_DATA_LEN];
} boot_frame_t;

int boot_protocol_decode(uint8_t *ascii, uint16_t ascii_len, boot_frame_t *frame);
uint8_t BootProtocol_ParseAsciiFrame(const uint8_t *ascii, uint16_t ascii_len, boot_frame_t *frame);

uint8_t BootProtocol_BuildBinaryFrame(uint16_t id,
                                      uint16_t cmd,
                                      const uint8_t *data,
                                      uint16_t data_len,
                                      uint8_t *out,
                                      uint16_t *out_len);

uint8_t BootProtocol_BuildAsciiFrame(uint16_t id,
                                     uint16_t cmd,
                                     const uint8_t *data,
                                     uint16_t data_len,
                                     uint8_t *out_ascii,
                                     uint16_t *out_ascii_len);

void BootProtocol_SendAck(uint16_t id, uint16_t cmd, uint8_t status);
void BootProtocol_SendData(uint16_t id, uint16_t cmd, const uint8_t *data, uint16_t len);

void BootProtocol_ProcessFrame(const boot_frame_t *frame);
int BootProtocol_ProcessFrameEx(const boot_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif

