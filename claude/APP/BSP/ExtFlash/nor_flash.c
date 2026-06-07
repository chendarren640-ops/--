/* SPDX-License-Identifier: MIT */

/**
 * @file    nor_flash.c
 * @brief   GD25Qxx serial NOR flash driver implementation.
 *
 * Uses SPI1 with DMA (CH3 RX, CH4 TX) for all transfers. The CS pin
 * is PB12. DMA buffers are allocated externally in the board setup.
 */

#include "nor_flash.h"

/** Extern SPI DMA buffers from board setup */
extern uint8_t g_Spi1TxBuf[FLASH_DMA_TXBUFSZ];
extern uint8_t g_Spi1RxBuf[FLASH_DMA_TXBUFSZ];

/**
 * @brief   Write a single page (must not cross a page boundary).
 * @param   pbuffer            Source data pointer
 * @param   write_addr         Destination flash address
 * @param   num_byte_to_write  Number of bytes (<= NOR_FLASH_PAGE_SZ)
 */
static void NorFlash_WritePage(uint8_t *pbuffer, uint32_t write_addr, uint16_t num_byte_to_write)
{
    NorFlash_WriteEnable();

    NOR_FLASH_CS_LOW();
    NorFlash_SendByteDma(FLASH_CMD_WRITE);
    NorFlash_SendByteDma((write_addr & 0xFF0000) >> 16);
    NorFlash_SendByteDma((write_addr & 0xFF00) >> 8);
    NorFlash_SendByteDma(write_addr & 0xFF);

    while (num_byte_to_write--)
    {
        NorFlash_SendByteDma(*pbuffer);
        pbuffer++;
    }

    NOR_FLASH_CS_HIGH();
    NorFlash_WaitBusy();
}

/**
 * @brief   Initialize SPI Flash (de-assert CS and enable SPI1).
 */
void NorFlash_Init(void)
{
    NOR_FLASH_CS_HIGH();
    spi_enable(NOR_FLASH_SPI);
}

/**
 * @brief   Erase a 4 KiB sector.
 * @param   sector_addr  Sector start address
 */
void NorFlash_EraseSector(uint32_t sector_addr)
{
    NorFlash_WriteEnable();

    NOR_FLASH_CS_LOW();
    NorFlash_SendByteDma(FLASH_CMD_ERASE_4K);
    NorFlash_SendByteDma((sector_addr & 0xFF0000) >> 16);
    NorFlash_SendByteDma((sector_addr & 0xFF00) >> 8);
    NorFlash_SendByteDma(sector_addr & 0xFF);
    NOR_FLASH_CS_HIGH();

    NorFlash_WaitBusy();
}

/**
 * @brief   Erase the entire flash chip.
 */
void NorFlash_EraseChip(void)
{
    NorFlash_WriteEnable();

    NOR_FLASH_CS_LOW();
    NorFlash_SendByteDma(FLASH_CMD_ERASE_CHIP);
    NOR_FLASH_CS_HIGH();

    NorFlash_WaitBusy();
}

/**
 * @brief   Write a block of data (handles page-crossing automatically).
 * @param   pbuffer            Source data pointer
 * @param   write_addr         Destination flash address
 * @param   num_byte_to_write  Number of bytes to write
 */
void NorFlash_Write(uint8_t *pbuffer, uint32_t write_addr, uint16_t num_byte_to_write)
{
    uint8_t num_of_page = 0, num_of_single = 0, addr = 0, count = 0, temp = 0;

    addr = write_addr % NOR_FLASH_PAGE_SZ;
    count = NOR_FLASH_PAGE_SZ - addr;
    num_of_page = num_byte_to_write / NOR_FLASH_PAGE_SZ;
    num_of_single = num_byte_to_write % NOR_FLASH_PAGE_SZ;

    if (0 == addr)
    {
        if (0 == num_of_page)
        {
            NorFlash_WritePage(pbuffer, write_addr, num_byte_to_write);
        }
        else
        {
            while (num_of_page--)
            {
                NorFlash_WritePage(pbuffer, write_addr, NOR_FLASH_PAGE_SZ);
                write_addr += NOR_FLASH_PAGE_SZ;
                pbuffer += NOR_FLASH_PAGE_SZ;
            }
            NorFlash_WritePage(pbuffer, write_addr, num_of_single);
        }
    }
    else
    {
        if (0 == num_of_page)
        {
            if (num_of_single > count)
            {
                temp = num_of_single - count;
                NorFlash_WritePage(pbuffer, write_addr, count);
                write_addr += count;
                pbuffer += count;
                NorFlash_WritePage(pbuffer, write_addr, temp);
            }
            else
            {
                NorFlash_WritePage(pbuffer, write_addr, num_byte_to_write);
            }
        }
        else
        {
            num_byte_to_write -= count;
            num_of_page = num_byte_to_write / NOR_FLASH_PAGE_SZ;
            num_of_single = num_byte_to_write % NOR_FLASH_PAGE_SZ;

            NorFlash_WritePage(pbuffer, write_addr, count);
            write_addr += count;
            pbuffer += count;

            while (num_of_page--)
            {
                NorFlash_WritePage(pbuffer, write_addr, NOR_FLASH_PAGE_SZ);
                write_addr += NOR_FLASH_PAGE_SZ;
                pbuffer += NOR_FLASH_PAGE_SZ;
            }

            if (0 != num_of_single)
            {
                NorFlash_WritePage(pbuffer, write_addr, num_of_single);
            }
        }
    }
}

/**
 * @brief   Read a block of data from flash.
 * @param   pbuffer           Destination buffer
 * @param   read_addr         Source flash address
 * @param   num_byte_to_read  Number of bytes to read
 */
void NorFlash_Read(uint8_t *pbuffer, uint32_t read_addr, uint16_t num_byte_to_read)
{
    NOR_FLASH_CS_LOW();
    NorFlash_SendByteDma(FLASH_CMD_READ);
    NorFlash_SendByteDma((read_addr & 0xFF0000) >> 16);
    NorFlash_SendByteDma((read_addr & 0xFF00) >> 8);
    NorFlash_SendByteDma(read_addr & 0xFF);

    while (num_byte_to_read--)
    {
        *pbuffer = NorFlash_SendByteDma(FLASH_DUMMY_BYTE);
        pbuffer++;
    }

    NOR_FLASH_CS_HIGH();
}

/**
 * @brief   Read JEDEC manufacturer/device ID.
 * @return  24-bit ID value
 */
uint32_t NorFlash_ReadId(void)
{
    uint32_t temp = 0, temp0 = 0, temp1 = 0, temp2 = 0;

    NOR_FLASH_CS_LOW();
    NorFlash_SendByteDma(FLASH_CMD_RDID);
    temp0 = NorFlash_SendByteDma(FLASH_DUMMY_BYTE);
    temp1 = NorFlash_SendByteDma(FLASH_DUMMY_BYTE);
    temp2 = NorFlash_SendByteDma(FLASH_DUMMY_BYTE);
    NOR_FLASH_CS_HIGH();

    temp = (temp0 << 16) | (temp1 << 8) | temp2;
    return temp;
}

/**
 * @brief   Begin a continuous read sequence at the given address.
 * @param   read_addr  Flash byte address
 */
void NorFlash_StartReadSeq(uint32_t read_addr)
{
    NOR_FLASH_CS_LOW();
    NorFlash_SendByteDma(FLASH_CMD_READ);
    NorFlash_SendByteDma((read_addr & 0xFF0000) >> 16);
    NorFlash_SendByteDma((read_addr & 0xFF00) >> 8);
    NorFlash_SendByteDma(read_addr & 0xFF);
}

/**
 * @brief   Send write-enable command.
 */
void NorFlash_WriteEnable(void)
{
    NOR_FLASH_CS_LOW();
    NorFlash_SendByteDma(FLASH_CMD_WREN);
    NOR_FLASH_CS_HIGH();
}

/**
 * @brief   Poll the write-in-progress flag until the device is idle.
 */
void NorFlash_WaitBusy(void)
{
    uint8_t flash_status = 0;

    NOR_FLASH_CS_LOW();
    NorFlash_SendByteDma(FLASH_CMD_RDSR);

    do
    {
        flash_status = NorFlash_SendByteDma(FLASH_DUMMY_BYTE);
    }
    while ((flash_status & FLASH_WIP_FLAG) == 0x01);

    NOR_FLASH_CS_HIGH();
}

/**
 * @brief   Send one byte via SPI DMA and return the received byte.
 * @param   byte  Byte to send
 * @return  Byte received from SPI
 */
uint8_t NorFlash_SendByteDma(uint8_t byte)
{
    g_Spi1TxBuf[0] = byte;

    dma_single_data_parameter_struct dma_init_struct;

    /* Configure DMA TX channel */
    dma_deinit(DMA0, DMA_CH4);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(NOR_FLASH_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_Spi1TxBuf;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = 1;
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(DMA0, DMA_CH4, &dma_init_struct);
    dma_channel_subperipheral_select(DMA0, DMA_CH4, DMA_SUBPERI0);

    /* Configure DMA RX channel */
    dma_deinit(DMA0, DMA_CH3);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(NOR_FLASH_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_Spi1RxBuf;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH3, &dma_init_struct);
    dma_channel_subperipheral_select(DMA0, DMA_CH3, DMA_SUBPERI0);

    /* Enable both DMA channels */
    dma_channel_enable(DMA0, DMA_CH3);
    dma_channel_enable(DMA0, DMA_CH4);

    /* Enable SPI DMA */
    spi_dma_enable(NOR_FLASH_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(NOR_FLASH_SPI, SPI_DMA_TRANSMIT);

    /* Wait for transfer completion */
    while (RESET == dma_flag_get(DMA0, DMA_CH3, DMA_FLAG_FTF));

    /* Disable DMA */
    spi_dma_disable(NOR_FLASH_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(NOR_FLASH_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(DMA0, DMA_CH3);
    dma_channel_disable(DMA0, DMA_CH4);

    /* Clear flags */
    dma_flag_clear(DMA0, DMA_CH3, DMA_FLAG_FTF);
    dma_flag_clear(DMA0, DMA_CH4, DMA_FLAG_FTF);

    return g_Spi1RxBuf[0];
}

/**
 * @brief   Send a 16-bit halfword via SPI DMA and return the received halfword.
 * @param   half_word  Halfword to send
 * @return  Halfword received from SPI
 */
uint16_t NorFlash_SendHalfDma(uint16_t half_word)
{
    uint16_t rx_data;

    g_Spi1TxBuf[0] = (uint8_t)(half_word >> 8);
    g_Spi1TxBuf[1] = (uint8_t)half_word;

    dma_single_data_parameter_struct dma_init_struct;

    /* Configure DMA TX channel */
    dma_deinit(DMA0, DMA_CH4);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(NOR_FLASH_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_Spi1TxBuf;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = 2;
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(DMA0, DMA_CH4, &dma_init_struct);
    dma_channel_subperipheral_select(DMA0, DMA_CH4, DMA_SUBPERI0);

    /* Configure DMA RX channel */
    dma_deinit(DMA0, DMA_CH3);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(NOR_FLASH_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_Spi1RxBuf;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH3, &dma_init_struct);
    dma_channel_subperipheral_select(DMA0, DMA_CH3, DMA_SUBPERI0);

    /* Enable both DMA channels */
    dma_channel_enable(DMA0, DMA_CH3);
    dma_channel_enable(DMA0, DMA_CH4);

    /* Enable SPI DMA */
    spi_dma_enable(NOR_FLASH_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(NOR_FLASH_SPI, SPI_DMA_TRANSMIT);

    /* Wait for transfer completion */
    while (RESET == dma_flag_get(DMA0, DMA_CH3, DMA_FLAG_FTF));

    /* Disable DMA */
    spi_dma_disable(NOR_FLASH_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(NOR_FLASH_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(DMA0, DMA_CH3);
    dma_channel_disable(DMA0, DMA_CH4);

    /* Clear flags */
    dma_flag_clear(DMA0, DMA_CH3, DMA_FLAG_FTF);
    dma_flag_clear(DMA0, DMA_CH4, DMA_FLAG_FTF);

    rx_data = (uint16_t)(g_Spi1RxBuf[0] << 8);
    rx_data |= g_Spi1RxBuf[1];

    return rx_data;
}

/**
 * @brief   Send and receive multiple bytes via SPI DMA.
 * @param   tx_buffer  Transmit data
 * @param   rx_buffer  Receive buffer
 * @param   size       Number of bytes to transfer
 */
void NorFlash_XferDma(uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t size)
{
    uint16_t i;

    if (size > FLASH_DMA_TXBUFSZ)
    {
        size = FLASH_DMA_TXBUFSZ;
    }

    for (i = 0; i < size; i++)
    {
        g_Spi1TxBuf[i] = tx_buffer[i];
    }

    dma_single_data_parameter_struct dma_init_struct;

    /* Configure DMA TX channel */
    dma_deinit(DMA0, DMA_CH4);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(NOR_FLASH_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_Spi1TxBuf;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = size;
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(DMA0, DMA_CH4, &dma_init_struct);
    dma_channel_subperipheral_select(DMA0, DMA_CH4, DMA_SUBPERI0);

    /* Configure DMA RX channel */
    dma_deinit(DMA0, DMA_CH3);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(NOR_FLASH_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)g_Spi1RxBuf;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH3, &dma_init_struct);
    dma_channel_subperipheral_select(DMA0, DMA_CH3, DMA_SUBPERI0);

    /* Enable both DMA channels */
    dma_channel_enable(DMA0, DMA_CH3);
    dma_channel_enable(DMA0, DMA_CH4);

    /* Enable SPI DMA */
    spi_dma_enable(NOR_FLASH_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(NOR_FLASH_SPI, SPI_DMA_TRANSMIT);

    /* Wait for transfer completion */
    while (RESET == dma_flag_get(DMA0, DMA_CH3, DMA_FLAG_FTF));

    /* Disable DMA */
    spi_dma_disable(NOR_FLASH_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(NOR_FLASH_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(DMA0, DMA_CH3);
    dma_channel_disable(DMA0, DMA_CH4);

    /* Clear flags */
    dma_flag_clear(DMA0, DMA_CH3, DMA_FLAG_FTF);
    dma_flag_clear(DMA0, DMA_CH4, DMA_FLAG_FTF);

    for (i = 0; i < size; i++)
    {
        rx_buffer[i] = g_Spi1RxBuf[i];
    }
}

/**
 * @brief   Wait for any pending DMA transfer to finish.
 */
void NorFlash_WaitDmaEnd(void)
{
    while (RESET == dma_flag_get(DMA0, DMA_CH3, DMA_FLAG_FTF));

    dma_flag_clear(DMA0, DMA_CH3, DMA_FLAG_FTF);
    dma_flag_clear(DMA0, DMA_CH4, DMA_FLAG_FTF);
}

/**
 * @brief   Self-test: erase, write, read back, verify.
 */
void NorFlash_Test(void)
{
    uint32_t flash_id;
    uint8_t write_buffer[NOR_FLASH_PAGE_SZ];
    uint8_t read_buffer[NOR_FLASH_PAGE_SZ];

    uint32_t test_addr = 0x000000;
    int erased_check_ok;
    const char *message = "Hello from GD32 to SPI FLASH! Microunion Studio Test - 12345.";
    uint16_t data_len;
    int i;

    Uart_Printf(DBG_UART, "SPI FLASH Test Start\r\n");

    /* 1. Initialize */
    NorFlash_Init();
    Uart_Printf(DBG_UART, "SPI Flash Initialized.\r\n");

    /* 2. Read ID */
    flash_id = NorFlash_ReadId();
    Uart_Printf(DBG_UART, "Flash ID: 0x%lX\r\n", flash_id);

    /* 3. Erase sector */
    Uart_Printf(DBG_UART, "Erasing sector at address 0x%lX...\r\n", test_addr);
    NorFlash_EraseSector(test_addr);
    Uart_Printf(DBG_UART, "Sector erased.\r\n");

    /* Verify erase */
    NorFlash_Read(read_buffer, test_addr, NOR_FLASH_PAGE_SZ);
    erased_check_ok = 1;
    for (i = 0; i < NOR_FLASH_PAGE_SZ; i++)
    {
        if (read_buffer[i] != 0xFF)
        {
            erased_check_ok = 0;
            break;
        }
    }
    if (erased_check_ok)
    {
        Uart_Printf(DBG_UART, "Erase check PASSED. Sector is all 0xFF.\r\n");
    }
    else
    {
        Uart_Printf(DBG_UART, "Erase check FAILED.\r\n");
    }

    /* 4. Write data */
    data_len = strlen(message);
    if (data_len >= NOR_FLASH_PAGE_SZ)
    {
        data_len = NOR_FLASH_PAGE_SZ - 1;
    }
    memset(write_buffer, 0, NOR_FLASH_PAGE_SZ);
    memcpy(write_buffer, message, data_len);
    write_buffer[data_len] = '\0';

    Uart_Printf(DBG_UART, "Writing data to address 0x%lX: \"%s\"\r\n", test_addr, write_buffer);
    NorFlash_Write(write_buffer, test_addr, NOR_FLASH_PAGE_SZ);
    Uart_Printf(DBG_UART, "Data written.\r\n");

    /* 5. Read back */
    Uart_Printf(DBG_UART, "Reading data from address 0x%lX...\r\n", test_addr);
    memset(read_buffer, 0x00, NOR_FLASH_PAGE_SZ);
    NorFlash_Read(read_buffer, test_addr, NOR_FLASH_PAGE_SZ);
    Uart_Printf(DBG_UART, "Data read: \"%.*s\"\r\n", NOR_FLASH_PAGE_SZ, read_buffer);

    /* 6. Verify */
    if (memcmp(write_buffer, read_buffer, NOR_FLASH_PAGE_SZ) == 0)
    {
        Uart_Printf(DBG_UART, "Data VERIFIED! Write and Read successful.\r\n");
    }
    else
    {
        Uart_Printf(DBG_UART, "Data VERIFICATION FAILED!\r\n");
    }

    Uart_Printf(DBG_UART, "SPI FLASH Test End\r\n");
}
