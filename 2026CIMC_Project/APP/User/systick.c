#include "systick.h"
volatile uint32_t g_sys_tick = 0;
void systick_config(void) {
    if (SysTick_Config(SystemCoreClock / 1000U)) while (1);
    NVIC_SetPriority(SysTick_IRQn, 0x00);
}
void delay_1ms(uint32_t count) {
    uint32_t start = g_sys_tick;
    while ((g_sys_tick - start) < count);
}
uint32_t millis(void) { return g_sys_tick; }
