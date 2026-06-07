/* SPDX-License-Identifier: MIT */

/**
 * @file    main.c
 * @brief   Application entry point and main control loop.
 */

#include "board_defs.h"
#include "flash_layout.h"

int main(void)
{
#ifdef __FIRMWARE_VERSION_DEFINE
    uint32_t fw_ver = 0;
#endif
    SCB->VTOR = APP1_BASE;

    Tick_Init();
    PerfCounter_Init(false);
    Tick_DelayMs(200); /* Wait download if SWIO be set to GPIO */

#ifdef __FIRMWARE_VERSION_DEFINE
    fw_ver = gd32f4xx_firmware_version_get();
#endif /* __FIRMWARE_VERSION_DEFINE */

    HalLed_Setup();
    HalBtn_Setup();
    HalOled_Setup();
    HalExtFlash_Setup();
    HalUart_Setup();
    HalExtAdc_Setup();
    HalAdc_Setup();
    HalDac_Setup();
    HalRtc_Setup();

    FlashFs_Init();
    Btn_Init();
    OLED_Init();
    OtaRx_Reset();
    AppCtrl_Init();
    Sched_Init();

    for (;;)
    {
        Sched_Run();
    }
}

#ifdef GD_ECLIPSE_GCC
/**
 * @brief   Retarget the C library printf function to the USART, in Eclipse GCC environment.
 * @param   ch  Character to output.
 * @return  The character written.
 */
int __io_putchar(int ch)
{
    RS485_DIR(1);
    usart_data_transmit(DBG_UART, (uint8_t)ch);
    while (RESET == usart_flag_get(DBG_UART, USART_FLAG_TBE));
    while (RESET == usart_flag_get(DBG_UART, USART_FLAG_TC));
    RS485_DIR(0);
    return ch;
}
#else
/**
 * @brief   Retarget the C library printf function to the USART.
 * @param   ch  Character to output.
 * @param   f   Output stream (unused).
 * @return  The character written.
 */
int fputc(int ch, FILE *f)
{
    RS485_DIR(1);
    usart_data_transmit(DBG_UART, (uint8_t)ch);
    while (RESET == usart_flag_get(DBG_UART, USART_FLAG_TBE));
    while (RESET == usart_flag_get(DBG_UART, USART_FLAG_TC));
    RS485_DIR(0);
    return ch;
}
#endif /* GD_ECLIPSE_GCC */
