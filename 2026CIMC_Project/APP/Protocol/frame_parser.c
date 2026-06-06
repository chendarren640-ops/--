#include "frame_parser.h"
#include "ascii_proto.h"
#include "crc16.h"

uint8_t  g_frame_ready = 0;
uint8_t  g_frame_buf[FRAME_BUF_SIZE];
uint16_t g_frame_len = 0;

static uint8_t ascii_buf[1024];
static uint16_t ascii_idx = 0;

void frame_parser_feed(uint8_t byte) {
    /* 收集ASCII字符, 检测起始A5B6 */
    ascii_buf[ascii_idx++] = byte;
    if (ascii_idx >= 4 &&
        ascii_buf[ascii_idx-4]=='A' && ascii_buf[ascii_idx-3]=='5' &&
        ascii_buf[ascii_idx-2]=='B' && ascii_buf[ascii_idx-1]=='6') {
        /* 从帧头开始 */
        if (ascii_idx > 4) {
            ascii_buf[0]='A';ascii_buf[1]='5';ascii_buf[2]='B';ascii_buf[3]='6';
            ascii_idx=4;
        }
    }
    /* 检测结束标志 B6A5 */
    if (ascii_idx >= 4 &&
        ascii_buf[ascii_idx-4]=='B' && ascii_buf[ascii_idx-3]=='6' &&
        ascii_buf[ascii_idx-2]=='A' && ascii_buf[ascii_idx-1]=='5') {
        /* 转换: ASCII hex → 原始字节 */
        ascii_buf[ascii_idx]='\0';
        g_frame_len = HexStrToBytes((char*)ascii_buf, ascii_idx, g_frame_buf);
        g_frame_ready = 1;
        ascii_idx = 0;
    }
    if (ascii_idx >= 1020) ascii_idx = 0; /* 防溢出 */
}

void frame_parser_process(void) {
    if (!g_frame_ready) return;
    g_frame_ready = 0;

    /* 最小帧: 起始2+ID2+类型1+命令2+长度1+版本1+CRC2+结束2 = 13字节 */
    if (g_frame_len < 13) return;

    /* CRC校验 (起始→内容末尾) */
    uint16_t calc_crc = CRC16_Modbus(g_frame_buf, g_frame_len - 4);
    uint16_t rcv_crc = (g_frame_buf[g_frame_len-4]<<8) | g_frame_buf[g_frame_len-3];
    if (calc_crc != rcv_crc) {
        /* TODO: 发送错误应答帧 FF EEEE */
        return;
    }
    /* TODO: 检查设备ID(非广播且不匹配则丢弃) */
    /* TODO: 分发到 cmd_handler */
}
