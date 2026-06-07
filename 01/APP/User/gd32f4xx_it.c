#include "gd32f4xx_it.h"
#include "systick.h"
#include "../Driver/USART/usart_drv.h"
#include "../Protocol/frame_parser.h"
#include "../Protocol/ascii_proto.h"

void NMI_Handler(void) {}
void HardFault_Handler(void) { while (1); }
void MemManage_Handler(void) { while (1); }
void BusFault_Handler(void)  { while (1); }
void UsageFault_Handler(void){ while (1); }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}

void SysTick_Handler(void) {
    static uint32_t tick_1s = 0;
    g_sys_tick++;
    if (++tick_1s >= 500) { tick_1s = 0; g_led_toggle_flag = 1; }
}

void USART0_IRQHandler(void) {
    if (RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE)) {
        uint8_t byte = usart_data_receive(USART0);
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);
        usart_data_transmit(USART0, byte);  /* 回显测试: 收到什么回什么, 验证USART0收发链路 */
#if PROTO_VIA_CH340
        /* CH340 模式下协议帧走 USART0 */
        frame_parser_feed(byte);
#else
        /* RS-485 模式下 USART0 仅用于调试 printf, 不喂帧解析器 */
        (void)byte;
#endif
    }
}

void USART1_IRQHandler(void) {
    if (RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE)) {
        uint8_t byte = usart_data_receive(USART1);
        usart_interrupt_flag_clear(USART1, USART_INT_FLAG_RBNE);

        /* 存入环形缓冲区 (调试用) */
        uint16_t next = (rx_head + 1) % RX_BUF_SIZE;
        if (next != rx_tail) {
            rx_buf[rx_head] = byte;
            rx_head = next;
        }

        /* 喂给帧解析器 — ASCII 十六进制流式解析 */
        frame_parser_feed(byte);
    }
}
