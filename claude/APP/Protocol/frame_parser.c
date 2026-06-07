/**
 * 2026 CIMC APP — ASCII Hex 帧解析器
 *
 * 上位机按 ASCII 十六进制字符串收发帧:
 *   帧头 A5B6 → 发送字符 'A','5','B','6'
 */
#include "frame_parser.h"
#include "ascii_proto.h"
#include "crc16.h"
#include "frame_builder.h"
#include "../Function/cmd_handler.h"
#include "../Driver/Flash/flash_param.h"

uint8_t  g_frame_ready = 0;
uint8_t  g_frame_buf[FRAME_BUF_SIZE];
uint16_t g_frame_len = 0;

static uint8_t ascii_buf[1024];
static uint16_t ascii_idx = 0;

void frame_parser_reset(void) {
    g_frame_ready = 0;
    ascii_idx = 0;
}

void frame_parser_feed(uint8_t byte) {
    ascii_buf[ascii_idx++] = byte;
    if (ascii_idx >= 4 &&
        ascii_buf[ascii_idx-4]=='A' && ascii_buf[ascii_idx-3]=='5' &&
        ascii_buf[ascii_idx-2]=='B' && ascii_buf[ascii_idx-1]=='6') {
        if (ascii_idx > 4) {
            ascii_buf[0]='A';ascii_buf[1]='5';ascii_buf[2]='B';ascii_buf[3]='6';
            ascii_idx=4;
        }
    }
    if (ascii_idx >= 4 &&
        ascii_buf[ascii_idx-4]=='B' && ascii_buf[ascii_idx-3]=='6' &&
        ascii_buf[ascii_idx-2]=='A' && ascii_buf[ascii_idx-1]=='5') {
        ascii_buf[ascii_idx]='\0';
        g_frame_len = HexStrToBytes((char*)ascii_buf, ascii_idx, g_frame_buf);
        g_frame_ready = 1;
        ascii_idx = 0;
    }
    if (ascii_idx >= 1020) ascii_idx = 0;
}

void frame_parser_process(void) {
    if (!g_frame_ready) return;
    g_frame_ready = 0;

    if (g_frame_len < 13) return;

    uint16_t calc_crc = CRC16_Modbus(g_frame_buf, g_frame_len - 4);
    uint16_t rcv_crc = (g_frame_buf[g_frame_len-4]<<8) | g_frame_buf[g_frame_len-3];
    if (calc_crc != rcv_crc) { Build_ErrorReply(); return; }

    uint16_t frame_id = (g_frame_buf[2]<<8) | g_frame_buf[3];
    if (frame_id != 0xFFFF && frame_id != g_param.device_id) return;

    uint8_t pay_len = g_frame_buf[7];
    if (9 + pay_len + 2 + 2 != g_frame_len) { Build_ErrorReply(); return; }

    cmd_dispatch(g_frame_buf, g_frame_len);
}
