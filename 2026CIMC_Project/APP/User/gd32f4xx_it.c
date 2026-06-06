#include "gd32f4xx_it.h"
#include "systick.h"
#include "usart1_drv.h"
volatile uint8_t g_sys_state = STATE_IDLE;
volatile uint8_t g_led_toggle_flag = 0;

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

void USART1_IRQHandler(void) {
    if (RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE)) {
        g_rx_byte = usart_data_receive(USART1);
        usart_interrupt_flag_clear(USART1, USART_INT_FLAG_RBNE);
        /* TODO: 送入帧解析器 frame_parser_feed(g_rx_byte) */
    }
}
