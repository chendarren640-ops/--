/* SPDX-License-Identifier: MIT */

/**
 * @file    periph_init.c
 * @brief   Board-level peripheral initialization for GD32F470.
 *
 * Provides setup and teardown functions for all on-board peripherals:
 * LEDs, buttons, OLED display (I2C), external SPI Flash, external SPI ADC,
 * internal ADC, DAC, RTC, UARTs (protocol + debug), power management
 * (deep-sleep entry/exit), and performance counter support.
 */

#include "board_defs.h"

/* ---- Globally shared buffers (extern to driver modules) ---- */

/** @brief OLED I2C command buffer: [control_byte, command_byte]. */
__IO uint8_t g_OledCmdBuf[2] = {0x00, 0x00};

/** @brief OLED I2C data buffer: [control_byte, data_byte]. */
__IO uint8_t g_OledDataBuf[2] = {0x40, 0x00};

/** @brief External ADC (GD30AD3344) SPI DMA TX buffer. */
uint8_t g_ExtAdcTxBuf[ARRAYSIZE] = {0};

/** @brief External ADC (GD30AD3344) SPI DMA RX buffer. */
uint8_t g_ExtAdcRxBuf[ARRAYSIZE] = {0};

/** @brief External SPI Flash DMA TX buffer. */
uint8_t g_ExtFlashTxBuf[ARRAYSIZE] = {0};

/** @brief External SPI Flash DMA RX buffer. */
uint8_t g_ExtFlashRxBuf[ARRAYSIZE] = {0};

/** @brief Protocol UART DMA circular RX buffer. */
uint8_t g_RxBuf[PROTO_RXBUF_SIZE];

/** @brief Debug UART DMA RX buffer. */
uint8_t g_DebugRxBuf[DBG_RXBUF_SIZE];

/** @brief ADC dual-channel raw sample buffer (DMA target). */
uint16_t g_AdcRaw[2];

/** @brief DAC waveform lookup table. */
uint16_t g_AdcConvBuf[ADC_SAMPLE_AVG] = {0};

/** @brief RTC initialization parameter structure. */
rtc_parameter_struct g_RtcInitParam;

/** @brief RTC alarm configuration structure. */
rtc_alarm_struct g_RtcAlarm;

/** @brief RTC asynchronous prescaler value. */
__IO uint32_t g_RtcPrescalerA = 0;

/** @brief RTC synchronous prescaler value. */
__IO uint32_t g_RtcPrescalerS = 0;

/** @brief RTC clock source selection flag (read from RCU_BDCTL). */
uint32_t g_RtcSrcFlag = 0;

/* ========================================================================
 *  LED
 * ======================================================================== */

/**
 * @brief   Initialize all system LEDs as push-pull outputs and turn them off.
 */
void HalLed_Setup(void)
{
    /* Enable the LED port clock */
    rcu_periph_clock_enable(LED_CLK_PORT);

    /* Configure LED GPIO pins as push-pull outputs */
    gpio_mode_set(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP,
                  LED_SYS_PIN | LED_SMP_PIN | LED_ALM_PIN |
                  LED_RUN_PIN | LED_TX_PIN  | LED_RX_PIN);
    gpio_output_options_set(LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            LED_SYS_PIN | LED_SMP_PIN | LED_ALM_PIN |
                            LED_RUN_PIN | LED_TX_PIN  | LED_RX_PIN);

    LED_SYS_OFF;
    LED_SMP_OFF;
    LED_ALM_OFF;
    LED_RUN_OFF;
    LED_TX_OFF;
    LED_RX_OFF;
}

/* ========================================================================
 *  Buttons
 * ======================================================================== */

/**
 * @brief   Initialize button GPIOs as inputs with pull-ups.
 */
void HalBtn_Setup(void)
{
    /* Enable button port clocks */
    rcu_periph_clock_enable(BTNE_CLK_PORT);
    rcu_periph_clock_enable(BTNB_CLK_PORT);
    rcu_periph_clock_enable(BTNA_CLK_PORT);

    /* Configure button pins as inputs with pull-up */
    gpio_mode_set(BTNE_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                  BTN1_PIN | BTN2_PIN | BTN3_PIN | BTN4_PIN | BTN5_PIN);
    gpio_mode_set(BTNB_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, BTN6_PIN);
    gpio_mode_set(BTNA_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, BTNW_PIN);
}

/**
 * @brief   Configure the wakeup key (PA0) EXTI line for deep-sleep wakeup.
 */
static void HalBtn_WakeupExtiSetup(void)
{
    rcu_periph_clock_enable(BTNA_CLK_PORT);
    rcu_periph_clock_enable(RCU_SYSCFG);

    gpio_mode_set(BTNA_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, BTNW_PIN);

    syscfg_exti_line_config(EXTI_SOURCE_GPIOA, EXTI_SOURCE_PIN0);
    exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_BOTH);
    exti_interrupt_flag_clear(EXTI_0);
    nvic_irq_enable(EXTI0_IRQn, 1U, 0U);
}

/* ========================================================================
 *  Deep-sleep support (static helpers)
 * ======================================================================== */

/**
 * @brief   Disable protocol UART and debug UART before entering deep-sleep.
 */
static void HalPwr_UartDisable(void)
{
    ota_uart_reset_state();

    usart_interrupt_disable(PROTO_UART, USART_INT_IDLE);
    nvic_irq_disable((IRQn_Type)PROTO_UART_IRQN);
    usart_dma_receive_config(PROTO_UART, USART_RECEIVE_DMA_DISABLE);
    dma_channel_disable(PROTO_UART_DMA, PROTO_UART_DMA_CH);
    usart_disable(PROTO_UART);

    if (DBG_UART != PROTO_UART)
    {
        usart_interrupt_disable(DBG_UART, USART_INT_IDLE);
        nvic_irq_disable(USART0_IRQn);
        usart_dma_receive_config(DBG_UART, USART_RECEIVE_DMA_DISABLE);
        dma_channel_disable(DBG_RX_DMA, DBG_RX_DMA_CH);
        usart_disable(DBG_UART);
    }
}

/**
 * @brief   Disable OLED I2C and set pins to analog for deep-sleep.
 */
static void HalPwr_OledDisable(void)
{
    OLED_Display_Off();

    i2c_dma_config(OLED_I2C, I2C_DMA_OFF);
    dma_channel_disable(DMA0, DMA_CH6);
    i2c_disable(OLED_I2C);

    gpio_mode_set(OLED_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  OLED_DAT_PIN | OLED_CLK_PIN);
}

/**
 * @brief   Disable SPI peripherals (Flash + external ADC) for deep-sleep.
 */
static void HalPwr_SpiDisable(void)
{
    EXT_FLASH_CS_HIGH();

    spi_dma_disable(EXT_FLASH_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(EXT_FLASH_SPI, SPI_DMA_TRANSMIT);
    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_TRANSMIT);

    dma_channel_disable(DMA0, DMA_CH3);
    dma_channel_disable(DMA0, DMA_CH4);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_DMA_CH_RX);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_DMA_CH_TX);

    spi_disable(EXT_FLASH_SPI);
    spi_disable(EXT_ADC_SPI);
}

/**
 * @brief   Set all GPIOs to their deep-sleep (low-power) state.
 */
static void HalPwr_GpioDeepSleep(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);

    LED_SYS_OFF;
    LED_SMP_OFF;
    LED_ALM_OFF;
    LED_RUN_OFF;
    LED_TX_OFF;
    LED_RX_OFF;

    gpio_mode_set(BTNE_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  BTN1_PIN | BTN2_PIN | BTN3_PIN | BTN4_PIN | BTN5_PIN);
    gpio_mode_set(BTNB_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, BTN6_PIN);

    gpio_mode_set(DBG_UART_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  DBG_UART_TX_PIN | DBG_UART_RX_PIN);
    gpio_mode_set(PROTO_UART_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  PROTO_UART_TX_PIN | PROTO_UART_RX_PIN);
    gpio_mode_set(RS485_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RS485_CS_PIN);
    gpio_output_options_set(RS485_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, RS485_CS_PIN);
    RS485_DIR(0);

    gpio_mode_set(OLED_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  OLED_DAT_PIN | OLED_CLK_PIN);

    gpio_mode_set(EXT_FLASH_SPI_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  EXT_FLASH_SPI_SCK | EXT_FLASH_SPI_MISO | EXT_FLASH_SPI_MOSI);
    gpio_mode_set(EXT_FLASH_SPI_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, EXT_FLASH_SPI_NSS);
    gpio_output_options_set(EXT_FLASH_SPI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, EXT_FLASH_SPI_NSS);
    GPIO_BOP(EXT_FLASH_SPI_PORT) = EXT_FLASH_SPI_NSS;

    gpio_mode_set(EXT_ADC_SPI_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  EXT_ADC_SPI_SCK | EXT_ADC_SPI_MISO | EXT_ADC_SPI_MOSI);
    gpio_mode_set(EXT_ADC_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, EXT_ADC_CS_PIN);
    gpio_output_options_set(EXT_ADC_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, EXT_ADC_CS_PIN);
    GPIO_BOP(EXT_ADC_CS_PORT) = EXT_ADC_CS_PIN;

#if ADC_VREF_SOURCE_PC2
    gpio_mode_set(ADC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  ADC1_PIN | ADC_CH1_PIN | ADC_VREF_PIN);
#else
    gpio_mode_set(ADC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  ADC1_PIN | ADC_CH1_PIN);
#endif
    gpio_mode_set(DAC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, DAC1_PIN);

    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_mode_set(GPIOD, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_2);

    gpio_mode_set(BTNA_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, BTNW_PIN);
}

/**
 * @brief   Restore all peripheral state after waking from deep-sleep.
 */
static void HalPwr_WakeupRestore(void)
{
    SystemInit();
    SystemCoreClockUpdate();
    Tick_Init();
    update_perf_counter();
    HalLed_Setup();
    HalBtn_Setup();
    HalUart_Setup();
    HalOled_Setup();
    OLED_Init();
    HalAdc_Setup();
    HalDac_Setup();
    HalExtFlash_Setup();
    HalExtAdc_Setup();
    FlashFs_Init();
    ota_uart_reset_state();
}

/* ========================================================================
 *  Deep-sleep public API
 * ======================================================================== */

/**
 * @brief   Enter deep-sleep mode. System wakes on PA0 EXTI.
 */
void HalPwr_DeepSleep(void)
{
    rcu_periph_clock_enable(RCU_PMU);

    __disable_irq();

    HalPwr_UartDisable();
    HalPwr_OledDisable();
    HalPwr_SpiDisable();

    adc_disable(ADC0);
    adc_dma_mode_disable(ADC0);
    dma_channel_disable(ADC_DMA, ADC_DMA_CHANNEL);
    dac_disable(DAC0, DAC_OUT0);
    timer_disable(TIMER5);

    HalPwr_GpioDeepSleep();
    HalBtn_WakeupExtiSetup();

    pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
    pmu_flag_clear(PMU_FLAG_RESET_STANDBY);

    before_cycle_counter_reconfiguration();
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    __enable_irq();

    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);

    HalPwr_WakeupRestore();
}

/**
 * @brief   Enter deep-sleep mode with RTC wakeup timer. System wakes on PA0 EXTI or RTC alarm.
 * @param   seconds  Wakeup timeout in seconds (minimum 1).
 */
void HalPwr_DeepSleepRtc(uint16_t seconds)
{
    rcu_periph_clock_enable(RCU_PMU);

    __disable_irq();

    HalPwr_UartDisable();
    HalPwr_OledDisable();
    HalPwr_SpiDisable();

    adc_disable(ADC0);
    adc_dma_mode_disable(ADC0);
    dma_channel_disable(ADC_DMA, ADC_DMA_CHANNEL);
    dac_disable(DAC0, DAC_OUT0);
    timer_disable(TIMER5);

    HalPwr_GpioDeepSleep();
    HalRtc_WakeupTimerStart(seconds);

    pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
    pmu_flag_clear(PMU_FLAG_RESET_STANDBY);

    before_cycle_counter_reconfiguration();
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    __enable_irq();

    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);

    HalRtc_WakeupTimerStop();
    HalPwr_WakeupRestore();
}

/* ========================================================================
 *  RTC wakeup timer helpers
 * ======================================================================== */

/**
 * @brief   Start the RTC wakeup timer.
 * @param   seconds  Timeout in seconds (clamped to >= 1).
 */
static void HalRtc_WakeupTimerStart(uint16_t seconds)
{
    if (seconds == 0U)
    {
        seconds = 1U;
    }

    exti_interrupt_flag_clear(EXTI_22);
    rtc_flag_clear(RTC_FLAG_WT);
    (void)rtc_wakeup_disable();
    (void)rtc_wakeup_clock_set(WAKEUP_CKSPRE);
    (void)rtc_wakeup_timer_set((uint16_t)(seconds - 1U));
    rtc_interrupt_enable(RTC_INT_WAKEUP);
    exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    nvic_irq_enable(RTC_WKUP_IRQn, 1U, 0U);
    rtc_wakeup_enable();
}

/**
 * @brief   Stop the RTC wakeup timer and disable its interrupt.
 */
static void HalRtc_WakeupTimerStop(void)
{
    (void)rtc_wakeup_disable();
    rtc_interrupt_disable(RTC_INT_WAKEUP);
    rtc_flag_clear(RTC_FLAG_WT);
    exti_interrupt_flag_clear(EXTI_22);
    nvic_irq_disable(RTC_WKUP_IRQn);
}

/* ========================================================================
 *  UART
 * ======================================================================== */

/**
 * @brief   Initialize the debug UART (TX only, no DMA RX in basic init).
 */
static void HalUart_DebugInit(void)
{
    rcu_periph_clock_enable(DBG_UART_PORT_RCU);
    rcu_periph_clock_enable(DBG_UART_RCU);

    gpio_af_set(DBG_UART_PORT, DBG_UART_AF, DBG_UART_TX_PIN);
    gpio_af_set(DBG_UART_PORT, DBG_UART_AF, DBG_UART_RX_PIN);

    gpio_mode_set(DBG_UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DBG_UART_TX_PIN);
    gpio_output_options_set(DBG_UART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, DBG_UART_TX_PIN);

    gpio_mode_set(DBG_UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DBG_UART_RX_PIN);
    gpio_output_options_set(DBG_UART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, DBG_UART_RX_PIN);

    usart_deinit(DBG_UART);
    usart_baudrate_set(DBG_UART, DBG_UART_BAUDRATE);
    usart_receive_config(DBG_UART, USART_RECEIVE_ENABLE);
    usart_transmit_config(DBG_UART, USART_TRANSMIT_ENABLE);
    usart_enable(DBG_UART);
}

/**
 * @brief   Initialize the protocol UART with circular DMA RX and RS485 direction control.
 */
static void HalUart_ProtoInit(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    /*
     * Protocol UART RX uses circular DMA. The foreground task polls the DMA
     * write index and forwards only newly received bytes to the frame parser.
     */
    rcu_periph_clock_enable(PROTO_UART_DMA_RCU);

    dma_deinit(PROTO_UART_DMA, PROTO_UART_DMA_CH);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)g_RxBuf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.number = PROTO_RXBUF_SIZE;
    dma_init_struct.periph_addr = PROTO_UART_RDATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(PROTO_UART_DMA, PROTO_UART_DMA_CH, &dma_init_struct);

    dma_circulation_enable(PROTO_UART_DMA, PROTO_UART_DMA_CH);
    dma_channel_subperipheral_select(PROTO_UART_DMA, PROTO_UART_DMA_CH, PROTO_UART_DMA_SUBPERI);
    dma_channel_enable(PROTO_UART_DMA, PROTO_UART_DMA_CH);

    rcu_periph_clock_enable(PROTO_UART_PORT_RCU);
    rcu_periph_clock_enable(PROTO_UART_RCU);
    rcu_periph_clock_enable(RS485_CS_PORT_RCU);

    gpio_af_set(PROTO_UART_PORT, PROTO_UART_AF, PROTO_UART_TX_PIN);
    gpio_af_set(PROTO_UART_PORT, PROTO_UART_AF, PROTO_UART_RX_PIN);

    gpio_mode_set(PROTO_UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, PROTO_UART_TX_PIN);
    gpio_output_options_set(PROTO_UART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, PROTO_UART_TX_PIN);

    gpio_mode_set(PROTO_UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, PROTO_UART_RX_PIN);
    gpio_output_options_set(PROTO_UART_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, PROTO_UART_RX_PIN);

    gpio_mode_set(RS485_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, RS485_CS_PIN);
    gpio_output_options_set(RS485_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_CS_PIN);
    RS485_DIR(0);

    usart_deinit(PROTO_UART);
    usart_baudrate_set(PROTO_UART, PROTO_UART_BAUDRATE);
    usart_receive_config(PROTO_UART, USART_RECEIVE_ENABLE);
    usart_transmit_config(PROTO_UART, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(PROTO_UART, USART_RECEIVE_DMA_ENABLE);
    usart_enable(PROTO_UART);

    /*
     * IDLE interrupt is enabled to keep the UART peripheral serviced; frame
     * assembly itself is done by polling the circular DMA buffer in uart_task().
     */
    nvic_irq_enable((IRQn_Type)PROTO_UART_IRQN, 0, 0);
    usart_interrupt_enable(PROTO_UART, USART_INT_IDLE);
}

/**
 * @brief   Initialize debug UART DMA RX (non-circular, single-shot per IDLE).
 */
static void HalUart_DebugDmaInit(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(DBG_RX_DMA_RCU);

    dma_deinit(DBG_RX_DMA, DBG_RX_DMA_CH);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)g_DebugRxBuf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.number = DBG_RXBUF_SIZE;
    dma_init_struct.periph_addr = DBG_UART_RDATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DBG_RX_DMA, DBG_RX_DMA_CH, &dma_init_struct);

    dma_circulation_disable(DBG_RX_DMA, DBG_RX_DMA_CH);
    dma_channel_subperipheral_select(DBG_RX_DMA, DBG_RX_DMA_CH, DBG_RX_DMA_SUBPERI);
    dma_channel_enable(DBG_RX_DMA, DBG_RX_DMA_CH);

    usart_dma_receive_config(DBG_UART, USART_RECEIVE_DMA_ENABLE);
    nvic_irq_enable(USART0_IRQn, 0, 1);
    usart_interrupt_enable(DBG_UART, USART_INT_IDLE);
}

/**
 * @brief   Initialize all UARTs: debug UART and protocol UART.
 */
void HalUart_Setup(void)
{
    if (DBG_UART != PROTO_UART)
    {
        HalUart_DebugInit();
        if (DBG_UART == USART0)
        {
            HalUart_DebugDmaInit();
        }
    }
    HalUart_ProtoInit();
}

/**
 * @brief   Re-arm the protocol UART DMA receiver (circular buffer reset).
 */
void ProtoUart_DmaRearm(void)
{
    dma_channel_disable(PROTO_UART_DMA, PROTO_UART_DMA_CH);
    dma_flag_clear(PROTO_UART_DMA, PROTO_UART_DMA_CH, DMA_FLAG_FTF);
    dma_transfer_number_config(PROTO_UART_DMA, PROTO_UART_DMA_CH, PROTO_RXBUF_SIZE);
    dma_channel_enable(PROTO_UART_DMA, PROTO_UART_DMA_CH);
}

/**
 * @brief   Get number of bytes received by the protocol UART DMA since last re-arm.
 * @return  Byte count received in the circular DMA buffer.
 */
uint32_t ProtoUart_DmaRxLen(void)
{
    uint32_t dma_left_cnt = dma_transfer_number_get(PROTO_UART_DMA, PROTO_UART_DMA_CH);
    return (PROTO_RXBUF_SIZE - dma_left_cnt);
}

/* ========================================================================
 *  OLED (I2C)
 * ======================================================================== */

/**
 * @brief   Initialize OLED I2C interface with DMA support.
 */
void HalOled_Setup(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    /* Enable GPIOB clock */
    rcu_periph_clock_enable(OLED_CLK_PORT);
    /* Enable I2C clock */
    rcu_periph_clock_enable(RCU_OLED_I2C);
    /* Enable DMA0 clock */
    rcu_periph_clock_enable(RCU_DMA0);

    /* Connect PB9 to I2C SDA */
    gpio_af_set(OLED_PORT, GPIO_AF_4, OLED_DAT_PIN);
    /* Connect PB8 to I2C SCL */
    gpio_af_set(OLED_PORT, GPIO_AF_4, OLED_CLK_PIN);

    gpio_mode_set(OLED_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, OLED_DAT_PIN);
    gpio_output_options_set(OLED_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_DAT_PIN);
    gpio_mode_set(OLED_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, OLED_CLK_PIN);
    gpio_output_options_set(OLED_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_CLK_PIN);

    /* Configure I2C clock */
    i2c_clock_config(OLED_I2C, 400000, I2C_DTCY_2);
    /* Configure I2C address */
    i2c_mode_addr_config(OLED_I2C, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, OLED_I2C_OWN_ADDRESS7);
    /* Enable I2C */
    i2c_enable(OLED_I2C);
    /* Enable acknowledge */
    i2c_ack_config(OLED_I2C, I2C_ACK_ENABLE);

    /* Configure DMA channel for I2C TX */
    dma_deinit(DMA0, DMA_CH6);

    dma_single_data_para_struct_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)g_OledDataBuf;  /* default source buffer */
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = 2;  /* control byte + payload byte */
    dma_init_struct.periph_addr = OLED_I2C_DATA_ADDRESS;  /* I2C data register */
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH6, &dma_init_struct);

    /* DMA configuration */
    dma_circulation_disable(DMA0, DMA_CH6);
    dma_channel_subperipheral_select(DMA0, DMA_CH6, DMA_SUBPERI1);  /* I2C TX */
}

/* ========================================================================
 *  External SPI Flash (GD25Qxx)
 * ======================================================================== */

/**
 * @brief   Initialize the external SPI Flash peripheral (GD25Qxx via SPI).
 */
void HalExtFlash_Setup(void)
{
    rcu_periph_clock_enable(EXT_FLASH_SPI_CLK_PORT);
    rcu_periph_clock_enable(RCU_EXT_FLASH_SPI);
    rcu_periph_clock_enable(RCU_DMA0);

    /* Configure SPI GPIO */
    gpio_af_set(EXT_FLASH_SPI_PORT, GPIO_AF_5,
                EXT_FLASH_SPI_SCK | EXT_FLASH_SPI_MISO | EXT_FLASH_SPI_MOSI);
    gpio_mode_set(EXT_FLASH_SPI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  EXT_FLASH_SPI_SCK | EXT_FLASH_SPI_MISO | EXT_FLASH_SPI_MOSI);
    gpio_output_options_set(EXT_FLASH_SPI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            EXT_FLASH_SPI_SCK | EXT_FLASH_SPI_MISO | EXT_FLASH_SPI_MOSI);

    /* Set NSS as GPIO */
    gpio_mode_set(EXT_FLASH_SPI_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, EXT_FLASH_SPI_NSS);
    gpio_output_options_set(EXT_FLASH_SPI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, EXT_FLASH_SPI_NSS);

    spi_parameter_struct spi_init_struct;

    /* Configure SPI parameter */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_8;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(EXT_FLASH_SPI, &spi_init_struct);

    /* Initialize SPI Flash device */
    spi_flash_init();
}

/* ========================================================================
 *  External SPI ADC (GD30AD3344)
 * ======================================================================== */

/**
 * @brief   Initialize the external SPI ADC (GD30AD3344) with DMA support.
 */
void HalExtAdc_Setup(void)
{
    rcu_periph_clock_enable(EXT_ADC_SPI_PORT_RCU);
    rcu_periph_clock_enable(EXT_ADC_CS_PORT_RCU);
    rcu_periph_clock_enable(RCU_EXT_ADC_SPI);
    rcu_periph_clock_enable(EXT_ADC_DMA_RCU);

    gpio_af_set(EXT_ADC_SPI_PORT, GPIO_AF_5,
                EXT_ADC_SPI_SCK | EXT_ADC_SPI_MISO | EXT_ADC_SPI_MOSI);
    gpio_mode_set(EXT_ADC_SPI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  EXT_ADC_SPI_SCK | EXT_ADC_SPI_MISO | EXT_ADC_SPI_MOSI);
    gpio_output_options_set(EXT_ADC_SPI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            EXT_ADC_SPI_SCK | EXT_ADC_SPI_MISO | EXT_ADC_SPI_MOSI);

    gpio_mode_set(EXT_ADC_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, EXT_ADC_CS_PIN);
    gpio_output_options_set(EXT_ADC_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, EXT_ADC_CS_PIN);
    EXT_ADC_CS_HIGH();

    spi_parameter_struct spi_init_struct;

    /* Configure SPI parameter */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = EXT_ADC_SPIMODE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_256;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(EXT_ADC_SPI, &spi_init_struct);

    /* Initialize SPI GD30AD3344 device */
    GD30AD3344_Init();
}

/* ========================================================================
 *  Internal ADC
 * ======================================================================== */

/**
 * @brief   Initialize the internal ADC (dual routine channel, DMA scan mode).
 */
void HalAdc_Setup(void)
{
    rcu_periph_clock_enable(ADC1_CLK_PORT);

    rcu_periph_clock_enable(RCU_ADC0);

    rcu_periph_clock_enable(RCU_DMA1);

    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);

    /* Configure the GPIO as analog mode */
#if ADC_VREF_SOURCE_PC2
    gpio_mode_set(ADC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  ADC1_PIN | ADC_CH1_PIN | ADC_VREF_PIN);
#else
    gpio_mode_set(ADC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  ADC1_PIN | ADC_CH1_PIN);
    adc_channel_16_to_18(ADC_TEMP_VREF_CHANNEL_SWITCH, ENABLE);
#endif

    /* ADC DMA channel configuration */
    dma_single_data_parameter_struct dma_single_data_parameter;

    /* ADC DMA channel configuration */
    dma_deinit(ADC_DMA, ADC_DMA_CHANNEL);

    /* Initialize DMA single data mode */
    dma_single_data_parameter.periph_addr = (uint32_t)(&ADC_RDATA(ADC0));
    dma_single_data_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_single_data_parameter.memory0_addr = (uint32_t)(g_AdcRaw);
    dma_single_data_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_single_data_parameter.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_single_data_parameter.direction = DMA_PERIPH_TO_MEMORY;
    dma_single_data_parameter.number = 2;
    dma_single_data_parameter.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(ADC_DMA, ADC_DMA_CHANNEL, &dma_single_data_parameter);
    dma_channel_subperipheral_select(ADC_DMA, ADC_DMA_CHANNEL, ADC_DMA_SUBPERI);

    /* Enable DMA circulation mode */
    dma_circulation_enable(ADC_DMA, ADC_DMA_CHANNEL);

    /* Enable DMA channel */
    dma_channel_enable(ADC_DMA, ADC_DMA_CHANNEL);

    /* ADC mode config */
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    /* ADC continuous function enable */
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    /* ADC scan mode enable */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    /* ADC data alignment config */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    /* ADC channel length config */
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 2);
    /* ADC routine channel config */
    adc_routine_channel_config(ADC0, 0, ADC_CH_POT, ADC_SAMPLETIME_15);
    adc_routine_channel_config(ADC0, 1, ADC_CH_DACFB, ADC_SAMPLETIME_15);
    /* ADC trigger config */
    adc_external_trigger_source_config(ADC0, ADC_ROUTINE_CHANNEL, ADC_EXTTRIG_ROUTINE_T0_CH0);
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);

    /* ADC DMA function enable */
    adc_dma_request_after_last_enable(ADC0);
    adc_dma_mode_enable(ADC0);

    /* Enable ADC interface */
    adc_enable(ADC0);
    /* Wait for ADC stability */
    Tick_DelayMs(1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);

    /* Enable ADC software trigger */
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
}

/* ========================================================================
 *  DAC
 * ======================================================================== */

/**
 * @brief   Configure TIMER5 as the DAC trigger source.
 */
static void HalDac_TimerConfig(void)
{
    timer_parameter_struct timer_initpara;

    /* TIMER deinitialize */
    timer_deinit(TIMER5);

    /* TIMER configuration */
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = 239;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 99;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;

    /* Initialize TIMER init parameter struct */
    timer_init(TIMER5, &timer_initpara);

    /* TIMER master mode output trigger source: Update event */
    timer_master_output_trigger_source_select(TIMER5, TIMER_TRI_OUT_SRC_UPDATE);

    /* Enable TIMER */
    timer_enable(TIMER5);
}

/**
 * @brief   Initialize the DAC with TIMER5 trigger for waveform generation.
 */
void HalDac_Setup(void)
{
    /* Enable GPIOA clock */
    rcu_periph_clock_enable(DAC1_CLK_PORT);
    /* Enable DAC clock */
    rcu_periph_clock_enable(RCU_DAC);
    /* Enable TIMER clock */
    rcu_periph_clock_enable(RCU_TIMER5);

    /* Configure PA4 as DAC output */
    gpio_mode_set(DAC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, DAC1_PIN);

    /* Initialize DAC */
    dac_deinit(DAC0);
    /* DAC trigger config */
    dac_trigger_source_config(DAC0, DAC_OUT0, DAC_TRIGGER_T5_TRGO);
    /* DAC trigger enable */
    dac_trigger_enable(DAC0, DAC_OUT0);
    /* DAC wave mode config */
    dac_wave_mode_config(DAC0, DAC_OUT0, DAC_WAVE_DISABLE);

    /* DAC enable */
    dac_enable(DAC0, DAC_OUT0);

    HalDac_TimerConfig();
}

/* ========================================================================
 *  RTC
 * ======================================================================== */

/**
 * @brief   Configure RTC clock source and prescalers based on crystal selection.
 */
static void HalRtc_PreConfig(void)
{
#if defined (RTC_CLOCK_SOURCE_IRC32K)
    rcu_osci_on(RCU_IRC32K);
    rcu_osci_stab_wait(RCU_IRC32K);
    rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);

    g_RtcPrescalerS = 0x13F;
    g_RtcPrescalerA = 0x63;
#elif defined (RTC_CLOCK_SOURCE_LXTAL)
    rcu_osci_on(RCU_LXTAL);
    rcu_osci_stab_wait(RCU_LXTAL);
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);

    g_RtcPrescalerS = 0xFF;
    g_RtcPrescalerA = 0x7F;
#else
#error RTC clock source should be defined.
#endif /* RTC_CLOCK_SOURCE_IRC32K */

    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();
}

/**
 * @brief   Set the RTC time/date to the default initial value.
 * @return  0 on success, -1 on RTC init failure.
 */
static int HalRtc_SetupTime(void)
{
    int ret = 0;
    /* Setup RTC time value */
    uint32_t tmp_hh = 0x23, tmp_mm = 0x59, tmp_ss = 0x50;

    g_RtcInitParam.factor_asyn = g_RtcPrescalerA;
    g_RtcInitParam.factor_syn = g_RtcPrescalerS;
    g_RtcInitParam.year = 0x25;
    g_RtcInitParam.day_of_week = RTC_SATURDAY;
    g_RtcInitParam.month = RTC_APR;
    g_RtcInitParam.date = 0x30;
    g_RtcInitParam.display_format = RTC_24HOUR;
    g_RtcInitParam.am_pm = RTC_AM;

    /* Current time input */
    g_RtcInitParam.hour = tmp_hh;
    g_RtcInitParam.minute = tmp_mm;
    g_RtcInitParam.second = tmp_ss;

    /* RTC current time configuration */
    if (ERROR == rtc_init(&g_RtcInitParam))
    {
        ret = -1;
    }
    else
    {
        RTC_BKP0 = BKP_VALUE;
    }
    return ret;
}

/**
 * @brief   Initialize the RTC peripheral with backup domain access.
 * @return  0 on success. Sets initial time each boot.
 */
int HalRtc_Setup(void)
{
    int ret = 0;

    /* Enable access to RTC registers in Backup domain */
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    HalRtc_PreConfig();

    /* Get RTC clock entry selection */
    g_RtcSrcFlag = GET_BITS(RCU_BDCTL, 8, 9);

    /* Set initial RTC each boot (board may not keep backup power) */
    HalRtc_SetupTime();

    /*
     * Optional: skip re-init if backup register is valid.
     * if ((BKP_VALUE != RTC_BKP0) || (0x00 == g_RtcSrcFlag)) {
     *     ret = HalRtc_SetupTime();
     * } else {
     *     if (RESET != rcu_flag_get(RCU_FLAG_PORRST)) {
     *         ret = 1;
     *     } else if (RESET != rcu_flag_get(RCU_FLAG_EPRST)) {
     *         ret = 2;
     *     }
     * }
     */

    rcu_all_reset_flag_clear();
    return ret;
}
