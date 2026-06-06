#ifndef __FRAME_BUILDER_H
#define __FRAME_BUILDER_H
#include "main.h"
#include "ascii_proto.h"
#include "crc16.h"

/* 构建应答帧并发送 */
void Build_Heartbeat(void);
void Build_OK_Reply(uint16_t cmd);
void Build_ErrorReply(void);
void Build_VersionReply(void);
void Build_TimeReply(uint32_t utc);
void Build_DataReply(uint16_t cmd, float value);
void Build_AutoSampleFrame(uint32_t utc, float ch0, float ch1);
void Build_IDReply(uint16_t dev_id);
void Build_BaudReply(uint8_t baud_code);
void Build_ThresholdReply(float ch0_threshold, float ch1_threshold);
void Build_AlarmString(uint32_t utc, uint8_t channel, float threshold, float actual);
#endif
