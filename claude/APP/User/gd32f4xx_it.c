#include "gd32f4xx_it.h"
#include "systick.h"
#include "../Driver/USART/usart_drv.h"
#include "../Protocol/frame_parser.h"

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

/* USART1 (RS-485) — RBNE+IDLE+错误处理 */
void USART1_IRQHandler(void) {
    /* 错误处理: 溢出/噪声/帧错 → 清标志, 防止 USART 锁死 */
    if (RESET != usart_flag_get(USART1, USART_FLAG_ORERR)) {
        usart_flag_clear(USART1, USART_FLAG_ORERR);
        usart_data_receive(USART1);  /* 清接收寄存器 */
    }
    if (RESET != usart_flag_get(USART1, USART_FLAG_NERR)) {
        usart_flag_clear(USART1, USART_FLAG_NERR);
    }
    if (RESET != usart_flag_get(USART1, USART_FLAG_FERR)) {
        usart_flag_clear(USART1, USART_FLAG_FERR);
    }

    if (RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE)) {
        uint8_t byte = usart_data_receive(USART1);
        uint16_t next = (rx_head + 1) % RX_BUF_SIZE;
        if (next != rx_tail) { rx_buf[rx_head] = byte; rx_head = next; }
        frame_parser_feed(byte);
    }
    if (RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE)) {
        usart_data_receive(USART1);  /* 清 IDLE 标志 */
    }
}
