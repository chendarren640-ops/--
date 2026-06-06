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

void frame_parser_feed(uint8_t byte) {
    /* 收集ASCII字符, 检测起始标志 A5B6 */
    ascii_buf[ascii_idx++] = byte;
    if (ascii_idx >= 4 &&
        ascii_buf[ascii_idx-4]=='A' && ascii_buf[ascii_idx-3]=='5' &&
        ascii_buf[ascii_idx-2]=='B' && ascii_buf[ascii_idx-1]=='6') {
        /* 在流中间发现帧头 → 重新同步到帧头位置 */
        if (ascii_idx > 4) {
            ascii_buf[0]='A';ascii_buf[1]='5';ascii_buf[2]='B';ascii_buf[3]='6';
            ascii_idx=4;
        }
    }
    /* 检测结束标志 B6A5 → 帧完整 */
    if (ascii_idx >= 4 &&
        ascii_buf[ascii_idx-4]=='B' && ascii_buf[ascii_idx-3]=='6' &&
        ascii_buf[ascii_idx-2]=='A' && ascii_buf[ascii_idx-1]=='5') {
        /* ASCII hex 字符串 → 原始字节帧 */
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

    /* === CRC16-Modbus 校验 (计算范围: 起始 → 内容末尾, 不含CRC和结束标志) === */
    uint16_t calc_crc = CRC16_Modbus(g_frame_buf, g_frame_len - 4);
    uint16_t rcv_crc = (g_frame_buf[g_frame_len-4]<<8) | g_frame_buf[g_frame_len-3];
    if (calc_crc != rcv_crc) {
        Build_ErrorReply();   /* CRC 错误 → FF EEEE */
        return;
    }

    /* === 设备 ID 校验 === */
    uint16_t frame_id = (g_frame_buf[2]<<8) | g_frame_buf[3];
    if (frame_id != 0xFFFF && frame_id != g_param.device_id) {
        return;   /* 非广播且 ID 不匹配 → 静默丢弃 */
    }

    /* === 长度合法性校验 === */
    uint8_t pay_len = g_frame_buf[7];
    if (9 + pay_len + 2 + 2 != g_frame_len) {
        Build_ErrorReply();   /* 长度不匹配 → FF EEEE (异常帧 K-02) */
        return;
    }

    /* === 分发到命令处理器 === */
    cmd_dispatch(g_frame_buf, g_frame_len);
}
