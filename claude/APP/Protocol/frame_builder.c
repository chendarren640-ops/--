#include "frame_builder.h"
#include "flash_param.h"

static uint8_t frm[256];
static uint16_t frm_len;

/* 助手: 填充帧并计算CRC+发送 */
static void frm_send(void) {
    uint16_t crc = CRC16_Modbus(frm, frm_len);
    frm[frm_len++] = (crc>>8)&0xFF;
    frm[frm_len++] = crc&0xFF;
    /* 结束标志 B6A5 */
    frm[frm_len++] = 0xB6; frm[frm_len++] = 0xA5;
    SendHexFrame(frm, frm_len);
}
static void frm_start(uint16_t dev_id, uint8_t type, uint16_t cmd, uint8_t pay_len) {
    frm[0]=0xA5;frm[1]=0xB6;
    frm[2]=(dev_id>>8)&0xFF;frm[3]=dev_id&0xFF;
    frm[4]=type;
    frm[5]=(cmd>>8)&0xFF;frm[6]=cmd&0xFF;
    frm[7]=pay_len;
    frm[8]=0x02; /* 协议版本 */
    frm_len=9+pay_len;
}

void Build_Heartbeat(void) {
    frm_start(g_param.device_id, 0x05, 0x8888, 0);
    frm_send();
}

void Build_OK_Reply(uint16_t cmd) {
    frm_start(g_param.device_id, 0x02, cmd, 1);
    frm[9]=0xFF;
    frm_send();
}

void Build_ErrorReply(void) {
    frm_start(g_param.device_id, 0xFF, 0xEEEE, 0);
    frm_send();
}

void Build_VersionReply(void) {
    frm_start(g_param.device_id, 0x02, 0x0104, 4);
    frm[9]=0x02;frm[10]=0x00;frm[11]=0x01;frm[12]=0x00; /* 2.0.1.0 */
    frm_send();
}

void Build_TimeReply(uint32_t utc) {
    frm_start(g_param.device_id, 0x02, 0x0106, 4);
    frm[9]=(utc>>24)&0xFF;frm[10]=(utc>>16)&0xFF;frm[11]=(utc>>8)&0xFF;frm[12]=utc&0xFF;
    frm_send();
}

void Build_DataReply(uint16_t cmd, float value) {
    frm_start(g_param.device_id, 0x02, cmd, 4);
    uint32_t ieee;
    memcpy(&ieee, &value, 4);
    frm[9]=(ieee>>24)&0xFF;frm[10]=(ieee>>16)&0xFF;frm[11]=(ieee>>8)&0xFF;frm[12]=ieee&0xFF;
    frm_send();
}

void Build_AutoSampleFrame(uint32_t utc, float ch0, float ch1) {
    frm_start(g_param.device_id, 0x02, 0x0302, 12);
    frm[9]=(utc>>24)&0xFF;frm[10]=(utc>>16)&0xFF;frm[11]=(utc>>8)&0xFF;frm[12]=utc&0xFF;
    uint32_t v;
    memcpy(&v,&ch0,4);frm[13]=(v>>24)&0xFF;frm[14]=(v>>16)&0xFF;frm[15]=(v>>8)&0xFF;frm[16]=v&0xFF;
    memcpy(&v,&ch1,4);frm[17]=(v>>24)&0xFF;frm[18]=(v>>16)&0xFF;frm[19]=(v>>8)&0xFF;frm[20]=v&0xFF;
    frm_send();
}

void Build_IDReply(uint16_t dev_id) {
    frm_start(dev_id, 0x02, 0x0111, 2);
    frm[9]=(dev_id>>8)&0xFF; frm[10]=dev_id&0xFF;
    frm_send();
}

void Build_BaudReply(uint8_t baud_code) {
    frm_start(g_param.device_id, 0x02, 0x0112, 1);
    frm[9]=baud_code;
    frm_send();
}

void Build_ThresholdReply(uint16_t cmd, float ch0_threshold, float ch1_threshold) {
    frm_start(g_param.device_id, 0x02, cmd, 8);
    uint32_t v;
    memcpy(&v, &ch0_threshold, 4);
    frm[9]=(v>>24)&0xFF;frm[10]=(v>>16)&0xFF;frm[11]=(v>>8)&0xFF;frm[12]=v&0xFF;
    memcpy(&v, &ch1_threshold, 4);
    frm[13]=(v>>24)&0xFF;frm[14]=(v>>16)&0xFF;frm[15]=(v>>8)&0xFF;frm[16]=v&0xFF;
    frm_send();
}

/* 告警字符串: "UTC时间|通道|阈值|实际值\r\n" — 不组帧, 纯字符串 (I-02) */
void Build_AlarmString(uint32_t utc, uint8_t channel, float threshold, float actual) {
    char buf[96];
    sprintf(buf, "%lu|CH%d|%.1f|%.1f\r\n",
            (unsigned long)utc, (int)channel,
            (double)threshold, (double)actual);
    USART1_SendString(buf);
}
