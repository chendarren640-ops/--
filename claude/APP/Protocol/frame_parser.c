/**
 * 2026 CIMC APP — ASCII 十六进制帧解析器
 *
 * 上位机按 ASCII 十六进制字符串收发帧:
 *   例如帧头 0xA5B6 → 上位机发送字符 'A','5','B','6'
 *   帧结构: A5B6 | ID(4) | TT(2) | CCCC(4) | LL(2) | VER(2) | 数据 | CRC(4) | B6A5
 *   全部以 ASCII 十六进制字符串形式传输
 */
#include "frame_parser.h"
#include "ascii_proto.h"
#include "crc16.h"
#include "frame_builder.h"
#include "../Function/cmd_handler.h"
#include "../Driver/Flash/flash_param.h"

volatile uint8_t  g_frame_ready = 0;   /* volatile: ISR 写, 主循环读 */
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
    uint8_t ready;

    __disable_irq();
    ready = g_frame_ready;
    if (ready) {
        g_frame_ready = 0;
        /* 拷贝到栈上防止 ISR 覆盖 */
        uint8_t local_buf[FRAME_BUF_SIZE];
        uint16_t local_len = g_frame_len;
        memcpy(local_buf, g_frame_buf, g_frame_len);
        __enable_irq();

        /* 最小帧: 起始2+ID2+类型1+命令2+长度1+版本1+CRC2+结束2 = 13字节 */
        if (local_len < 13) return;

        /* === CRC16-Modbus 校验 (计算范围: 起始 → 内容末尾, 不含CRC和结束标志) === */
        uint16_t calc_crc = CRC16_Modbus(local_buf, local_len - 4);
        uint16_t rcv_crc = (local_buf[local_len-4]<<8) | local_buf[local_len-3];
        if (calc_crc != rcv_crc) {
            Build_ErrorReply();   /* CRC 错误 → FF EEEE */
            return;
        }

        /* === 设备 ID 校验 === */
        uint16_t frame_id = (local_buf[2]<<8) | local_buf[3];
        if (frame_id != 0xFFFF && frame_id != g_param.device_id) {
            return;   /* 非广播且 ID 不匹配 → 静默丢弃 */
        }

        /* === 长度合法性校验 === */
        uint8_t pay_len = local_buf[7];
        if (9 + pay_len + 2 + 2 != local_len) {
            Build_ErrorReply();   /* 长度不匹配 → FF EEEE (异常帧 K-02) */
            return;
        }

        /* === 分发到命令处理器 === */
        cmd_dispatch(local_buf, local_len);
    } else {
        __enable_irq();
    }
}
