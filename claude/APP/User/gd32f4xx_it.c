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
    static uint32_t tick_500ms = 0;
    g_sys_tick++;
    if (++tick_500ms >= 500) { tick_500ms = 0; g_led_toggle_flag = 1; }
}

/* USART0 (CH340) — 调试 printf + CH340 模式下协议中转 */
void USART0_IRQHandler(void) {
    if (RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE)) {
        uint8_t byte = usart_data_receive(USART0);
#if PROTO_VIA_CH340
        /* CH340 模式: USART0 是协议口 */
        frame_parser_feed(byte);
#else
        /* RS-485 模式: USART0 仅调试, 丢弃所有接收字节 */
        (void)byte;
#endif
    }
}

/* USART1 (RS-485/USART0复用) — 帧接收 */
void USART1_IRQHandler(void) {
    if (RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE)) {
        uint8_t byte = usart_data_receive(USART1);

        /* 存入环形缓冲区 (调试用) */
        uint16_t next = (rx_head + 1) % RX_BUF_SIZE;
        if (next != rx_tail) {
            rx_buf[rx_head] = byte;
            rx_head = next;
        }

#if !PROTO_VIA_CH340
        /* RS-485 模式下 USART1 是协议口 */
        frame_parser_feed(byte);
#else
        /* CH340 模式下协议走 USART0, USART1 仅中转 */
        (void)byte;
#endif
    }
}

/* RTC 唤醒中断 — 深睡眠定时唤醒 */
void RTC_WKUP_IRQHandler(void) {
    if (RESET != rtc_flag_get(RTC_FLAG_WT)) {
        rtc_flag_clear(RTC_FLAG_WT);
        exti_interrupt_flag_clear(EXTI_22);
    }
}

/* RTC 闹钟中断 */
void RTC_Alarm_IRQHandler(void) {
    if (RESET != rtc_flag_get(RTC_FLAG_ALRM0)) {
        rtc_flag_clear(RTC_FLAG_ALRM0);
        exti_interrupt_flag_clear(EXTI_17);
    }
}
