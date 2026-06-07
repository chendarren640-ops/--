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
    SendHexFrame(frm, frm_len);  /* 发送转 ASCII hex, 接收保持二进制 */
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

/* MicroLib 浮点辅助: float → "XX.X" 字符串 (最多 2 位小数) */
static void ftoa1(float val, char *out) {
    if (val < 0) { *out++ = '-'; val = -val; }
    int32_t ip = (int32_t)val;
    uint8_t dp = (uint8_t)((val - (float)ip) * 10.0f + 0.5f);
    if (dp >= 10) { ip++; dp = 0; }
    if (ip >= 100) { *out++ = (char)('0' + (ip / 100) % 10); }
    if (ip >= 10)  { *out++ = (char)('0' + (ip / 10) % 10); }
    *out++ = (char)('0' + (ip % 10));
    *out++ = '.';
    *out++ = (char)('0' + dp);
    *out = '\0';
}

/* 告警字符串: "时间 | CHx | 阈值 | 实际值\r\n" — PDF 格式, 不组帧 (I-02) */
void Build_AlarmString(uint32_t utc, uint8_t channel, float threshold, float actual) {
    char buf[128], tmp[16];
    uint32_t days = utc / 86400;
    uint32_t secs = utc % 86400;
    uint16_t y = 1970;
    while(1) { uint16_t d = ((y%4==0&&y%100!=0)||y%400==0)?366:365; if(days<d)break; days-=d; y++; }
    uint8_t mdays[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if((y%4==0&&y%100!=0)||y%400==0)mdays[1]=29;
    uint8_t m=0; while(days>=mdays[m]){days-=mdays[m];m++;}

    /* 日期时间部分 */
    uint8_t idx = (uint8_t)sprintf(buf, "%04u-%02u-%02u %02u:%02u:%02u | CH%u | ",
        y, m+1, (uint8_t)(days+1),
        (uint8_t)(secs/3600), (uint8_t)((secs%3600)/60), (uint8_t)(secs%60), channel);

    /* 阈值 (手动浮点) */
    ftoa1(threshold, tmp);
    for (uint8_t i = 0; tmp[i]; i++) buf[idx++] = tmp[i];
    buf[idx++] = ' '; buf[idx++] = '|'; buf[idx++] = ' ';

    /* 实际值 */
    ftoa1(actual, tmp);
    for (uint8_t i = 0; tmp[i]; i++) buf[idx++] = tmp[i];
    buf[idx++] = '\r'; buf[idx++] = '\n';
    buf[idx] = '\0';

    ProtoSendString(buf);
}
