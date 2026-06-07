/* SPDX-License-Identifier: MIT */

/**
 * @file    ext_adc.c
 * @brief   GD30AD3344 24-bit delta-sigma ADC driver implementation.
 *
 * Communicates over SPI0 with DMA (DMA1, CH0 RX / CH5 TX), CS on PE8.
 */

#include "ext_adc.h"

/** Extern SPI DMA buffers from board setup */
extern uint8_t g_ExtAdcTxBuf[FLASH_DMA_TXBUFSZ];
extern uint8_t g_ExtAdcRxBuf[FLASH_DMA_TXBUFSZ];

static void ExtAdc_ConfigSpi(uint32_t frame_size);
static uint16_t ExtAdc_SendWordTriple(uint16_t first_word, uint16_t second_word,
                                       uint16_t third_word, uint16_t *first_rx,
                                       uint16_t *second_rx);

/**
 * @brief   Send one byte via SPI DMA.
 * @param   byte  Byte to send
 * @return  Byte received from SPI
 */
uint8_t ExtAdc_SendByteDma(uint8_t byte)
{
    g_ExtAdcTxBuf[0] = byte;

    dma_single_data_parameter_struct dma_init_struct;

    dma_deinit(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(EXT_ADC_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_ExtAdcTxBuf;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = 1;
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, &dma_init_struct);
    dma_channel_subperipheral_select(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, EXT_ADC_SPI_DMA_SUBPERI);

    dma_deinit(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(EXT_ADC_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_ExtAdcRxBuf;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, &dma_init_struct);
    dma_channel_subperipheral_select(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, EXT_ADC_SPI_DMA_SUBPERI);

    dma_channel_enable(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_channel_enable(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);

    spi_dma_enable(EXT_ADC_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(EXT_ADC_SPI, SPI_DMA_TRANSMIT);

    while (RESET == dma_flag_get(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, DMA_FLAG_FTF));

    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);

    dma_flag_clear(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, DMA_FLAG_FTF);
    dma_flag_clear(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, DMA_FLAG_FTF);

    return g_ExtAdcRxBuf[0];
}

/**
 * @brief   Send a 16-bit halfword via SPI DMA.
 * @param   half_word  Halfword to send
 * @return  Halfword received from SPI
 */
uint16_t ExtAdc_SendHalfDma(uint16_t half_word)
{
    EXT_ADC_CS_LOW();
    uint16_t rx_data;

    g_ExtAdcTxBuf[0] = (uint8_t)(half_word >> 8);
    g_ExtAdcTxBuf[1] = (uint8_t)half_word;

    dma_single_data_parameter_struct dma_init_struct;

    dma_deinit(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(EXT_ADC_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_ExtAdcTxBuf;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = 2;
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, &dma_init_struct);
    dma_channel_subperipheral_select(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, EXT_ADC_SPI_DMA_SUBPERI);

    dma_deinit(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(EXT_ADC_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_ExtAdcRxBuf;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, &dma_init_struct);
    dma_channel_subperipheral_select(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, EXT_ADC_SPI_DMA_SUBPERI);

    dma_channel_enable(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_channel_enable(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);

    spi_dma_enable(EXT_ADC_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(EXT_ADC_SPI, SPI_DMA_TRANSMIT);

    while (RESET == dma_flag_get(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, DMA_FLAG_FTF));

    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);

    dma_flag_clear(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, DMA_FLAG_FTF);
    dma_flag_clear(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, DMA_FLAG_FTF);

    rx_data = (uint16_t)(g_ExtAdcRxBuf[0] << 8);
    rx_data |= g_ExtAdcRxBuf[1];
    EXT_ADC_CS_HIGH();
    return rx_data;
}

/**
 * @brief   Send two 16-bit words in one CS window and return the second response.
 * @param   first_word   First word to send
 * @param   second_word  Second word to send
 * @return  Second received halfword
 */
uint16_t ExtAdc_SendWordPair(uint16_t first_word, uint16_t second_word)
{
    uint16_t rx_data;

    EXT_ADC_CS_LOW();

    g_ExtAdcTxBuf[0] = (uint8_t)(first_word >> 8);
    g_ExtAdcTxBuf[1] = (uint8_t)first_word;
    g_ExtAdcTxBuf[2] = (uint8_t)(second_word >> 8);
    g_ExtAdcTxBuf[3] = (uint8_t)second_word;

    dma_single_data_parameter_struct dma_init_struct;

    dma_deinit(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(EXT_ADC_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_ExtAdcTxBuf;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = 4;
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, &dma_init_struct);
    dma_channel_subperipheral_select(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, EXT_ADC_SPI_DMA_SUBPERI);

    dma_deinit(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(EXT_ADC_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_ExtAdcRxBuf;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, &dma_init_struct);
    dma_channel_subperipheral_select(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, EXT_ADC_SPI_DMA_SUBPERI);

    dma_channel_enable(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_channel_enable(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);

    spi_dma_enable(EXT_ADC_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(EXT_ADC_SPI, SPI_DMA_TRANSMIT);

    while (RESET == dma_flag_get(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, DMA_FLAG_FTF));

    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);

    dma_flag_clear(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, DMA_FLAG_FTF);
    dma_flag_clear(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, DMA_FLAG_FTF);

    rx_data = (uint16_t)(g_ExtAdcRxBuf[2] << 8);
    rx_data |= g_ExtAdcRxBuf[3];

    EXT_ADC_CS_HIGH();

    return rx_data;
}

/**
 * @brief   Send three 16-bit words and return all three responses.
 */
static uint16_t ExtAdc_SendWordTriple(uint16_t first_word, uint16_t second_word,
                                       uint16_t third_word, uint16_t *first_rx,
                                       uint16_t *second_rx)
{
    uint16_t third_rx;

    EXT_ADC_CS_LOW();

    g_ExtAdcTxBuf[0] = (uint8_t)(first_word >> 8);
    g_ExtAdcTxBuf[1] = (uint8_t)first_word;
    g_ExtAdcTxBuf[2] = (uint8_t)(second_word >> 8);
    g_ExtAdcTxBuf[3] = (uint8_t)second_word;
    g_ExtAdcTxBuf[4] = (uint8_t)(third_word >> 8);
    g_ExtAdcTxBuf[5] = (uint8_t)third_word;

    dma_single_data_parameter_struct dma_init_struct;

    dma_deinit(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(EXT_ADC_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_ExtAdcTxBuf;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = 6;
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, &dma_init_struct);
    dma_channel_subperipheral_select(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, EXT_ADC_SPI_DMA_SUBPERI);

    dma_deinit(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(EXT_ADC_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_ExtAdcRxBuf;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, &dma_init_struct);
    dma_channel_subperipheral_select(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, EXT_ADC_SPI_DMA_SUBPERI);

    dma_channel_enable(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_channel_enable(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);

    spi_dma_enable(EXT_ADC_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(EXT_ADC_SPI, SPI_DMA_TRANSMIT);

    while (RESET == dma_flag_get(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, DMA_FLAG_FTF));

    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);

    dma_flag_clear(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, DMA_FLAG_FTF);
    dma_flag_clear(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, DMA_FLAG_FTF);

    if (first_rx != NULL)
    {
        *first_rx = (uint16_t)(g_ExtAdcRxBuf[0] << 8);
        *first_rx |= g_ExtAdcRxBuf[1];
    }
    if (second_rx != NULL)
    {
        *second_rx = (uint16_t)(g_ExtAdcRxBuf[2] << 8);
        *second_rx |= g_ExtAdcRxBuf[3];
    }
    third_rx = (uint16_t)(g_ExtAdcRxBuf[4] << 8);
    third_rx |= g_ExtAdcRxBuf[5];

    EXT_ADC_CS_HIGH();

    return third_rx;
}

/**
 * @brief   Reconfigure SPI peripheral for a different frame size.
 * @param   frame_size  SPI_FRAMESIZE_8BIT or SPI_FRAMESIZE_16BIT
 */
static void ExtAdc_ConfigSpi(uint32_t frame_size)
{
    spi_parameter_struct spi_init_struct;

    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(EXT_ADC_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH);
    dma_channel_disable(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH);

    spi_i2s_deinit(EXT_ADC_SPI);
    spi_struct_para_init(&spi_init_struct);

    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = frame_size;
    spi_init_struct.clock_polarity_phase = EXT_ADC_SPI_MODE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_256;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(EXT_ADC_SPI, &spi_init_struct);
    spi_enable(EXT_ADC_SPI);
    (void)spi_i2s_data_receive(EXT_ADC_SPI);
}

/**
 * @brief   Wait until pending DMA transfer completes.
 */
void ExtAdc_WaitDmaEnd(void)
{
    while (RESET == dma_flag_get(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, DMA_FLAG_FTF));

    dma_flag_clear(EXT_ADC_DMA, EXT_ADC_SPI_RX_DMA_CH, DMA_FLAG_FTF);
    dma_flag_clear(EXT_ADC_DMA, EXT_ADC_SPI_TX_DMA_CH, DMA_FLAG_FTF);
}

ExtAdcReg_s g_ExtAdcCfg;
uint16_t g_ExtAdcRawCode = 0U;

/**
 * @brief   Initialize the GD30AD3344 with default configuration.
 */
void ExtAdc_Init(void)
{
    g_ExtAdcCfg.SS         = EXTADC_OS_DISABLE;
    g_ExtAdcCfg.MUX        = EXTADC_CH_AIN0_GND;
    g_ExtAdcCfg.PGA        = EXTADC_PGA_2V048;
    g_ExtAdcCfg.MODE       = EXTADC_MODE_CONTINUOUS;
    g_ExtAdcCfg.DR         = EXTADC_DR_25SPS;
    g_ExtAdcCfg.RESERVED_1 = 0U;
    g_ExtAdcCfg.PULL_UP_EN = EXTADC_PULLUP_ENABLE;
    g_ExtAdcCfg.NOP        = EXTADC_NOP_VALID_UPDATE;
    g_ExtAdcCfg.RESERVED   = 1U;

    spi_enable(EXT_ADC_SPI);
    ExtAdc_SendHalfDma(EXTADC_REG_VAL);
    Uart_Printf(DBG_UART, "0x%4X\r\n", EXTADC_REG_VAL);
}

/**
 * @brief   Read the current configuration register value.
 * @return  Raw configuration register value
 */
uint16_t ExtAdc_ReadCfg(void)
{
    g_ExtAdcCfg.NOP = EXTADC_NOP_VALID_NO_UPD;
    return ExtAdc_SendWordPair(EXTADC_REG_VAL, EXTADC_REG_VAL);
}

/**
 * @brief   Enable the external reference register bit.
 * @return  Register value read back after write
 */
uint16_t ExtAdc_ConfigRef(void)
{
    uint16_t reg_addr = EXTADC_EXTREF_REG_ADDR;
    uint16_t proc_cmd;
    uint16_t proc_addr;
    uint16_t proc_data;
    uint16_t read_cmd;
    uint16_t read_addr;
    uint16_t read_value;
    uint16_t write_value;
    uint16_t write_cmd;
    uint16_t write_addr;
    uint16_t write_data;
    uint16_t verify_cmd;
    uint16_t verify_addr;
    uint16_t verify_value;

    ExtAdc_ConfigSpi(SPI_FRAMESIZE_8BIT);

    proc_data = ExtAdc_SendWordTriple(EXTADC_EXTREG_WRITE_CMD,
                                       EXTADC_EXTPROC_REG_ADDR,
                                       EXTADC_EXTPROC_UNLOCK,
                                       &proc_cmd,
                                       &proc_addr);
    delay_1ms(1);

    read_value = ExtAdc_SendWordTriple(EXTADC_EXTREG_READ_CMD,
                                        reg_addr,
                                        0x0000U,
                                        &read_cmd,
                                        &read_addr);
    delay_1ms(1);

    write_value = read_value | EXTADC_EXTREF_EN_BIT;

    write_data = ExtAdc_SendWordTriple(EXTADC_EXTREG_WRITE_CMD,
                                        reg_addr,
                                        write_value,
                                        &write_cmd,
                                        &write_addr);
    delay_1ms(1);

    verify_value = ExtAdc_SendWordTriple(EXTADC_EXTREG_READ_CMD,
                                          reg_addr,
                                          0x0000U,
                                          &verify_cmd,
                                          &verify_addr);

    return verify_value;
}

/**
 * @brief   Map PGA enum to full-scale range in volts.
 * @param   PGA  PGA setting
 * @return  Full-scale range in volts
 */
static float ExtAdc_PgaToRange(ExtAdcPga_e PGA)
{
    switch (PGA)
    {
    case EXTADC_PGA_6V144:
        return 6.144f;
    case EXTADC_PGA_4V096:
        return 4.096f;
    case EXTADC_PGA_2V048:
        return 2.048f;
    case EXTADC_PGA_1V024:
        return 1.024f;
    case EXTADC_PGA_0V512:
        return 0.512f;
    case EXTADC_PGA_0V256:
        return 0.256f;
    case EXTADC_PGA_0V064:
        return 0.064f;
    default:
        return 2.048f;
    }
}

/**
 * @brief   Read one ADC channel and convert to voltage.
 * @param   CH   Input channel
 * @param   Ref  PGA full-scale range
 * @return  Converted voltage in volts
 */
float ExtAdc_Read(ExtAdcCh_e CH, ExtAdcPga_e Ref)
{
    uint16_t raw_data;
    float result = 0.0f;

    g_ExtAdcCfg.MUX = CH;
    g_ExtAdcCfg.PGA = Ref;

    raw_data = ExtAdc_SendHalfDma(EXTADC_REG_VAL);
    g_ExtAdcRawCode = raw_data;

    result = (float)((int16_t)raw_data) * 2.055f / 32767.0f;

    return (float)result;
}
