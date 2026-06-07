/* SPDX-License-Identifier: MIT */

/**
 * @file    oled.c
 * @brief   SSD1306 0.91" OLED driver — I2C DMA transport layer.
 *
 * This library drives a 128x32 SSD1306 OLED display over I2C0 using DMA
 * for command and data transmission.
 */

#include "oled.h"
#include "oledfont.h"
#include "gd32f4xx_i2c.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_dma.h"
#include "perf_counter.h"

/** OLED DMA buffers (extern from periph_init.c) */
extern uint8_t g_OledCmdBuf[2];
extern uint8_t g_OledDataBuf[2];

#define DISPLAY_I2C_TIMEOUT    100000U
#define DISPLAY_I2C_ADDR_WR    0x78U

/** Global flag: 1 = display is operational, 0 = bus error / offline */
uint8_t m_DisplayReady = 1U;

/**
 * @brief   Software I2C bus reset sequence.
 *
 * Pulses SCL 9 times followed by an explicit STOP to recover from a
 * stuck SDA line, then re-initializes I2C0 hardware.
 */
static void I2cBus_Reset(void)
{
    uint8_t i;

    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_8 | GPIO_PIN_9);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8 | GPIO_PIN_9);

    /* Release SCL/SDA first */
    gpio_bit_set(GPIOB, GPIO_PIN_8 | GPIO_PIN_9);
    delay_ms(10);

    /* Send 9 SCL pulses to release a pulled-low SDA */
    for (i = 0; i < 9; i++)
    {
        gpio_bit_reset(GPIOB, GPIO_PIN_8);
        delay_ms(5);
        gpio_bit_set(GPIOB, GPIO_PIN_8);
        delay_ms(5);
    }

    /* Generate an explicit STOP */
    gpio_bit_set(GPIOB, GPIO_PIN_8);
    gpio_bit_reset(GPIOB, GPIO_PIN_9);
    delay_ms(5);
    gpio_bit_set(GPIOB, GPIO_PIN_9);
    delay_ms(5);

    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_8);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_8);

    i2c_deinit(I2C0);
    i2c_clock_config(I2C0, 400000, I2C_DTCY_2);
    i2c_mode_addr_config(I2C0, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0x72);
    i2c_enable(I2C0);
    i2c_ack_config(I2C0, I2C_ACK_ENABLE);

    delay_ms(10);
}

/**
 * @brief   Poll an I2C flag until it is set or timeout expires.
 * @param   i2c_periph  I2C peripheral base
 * @param   flag        I2C flag to poll
 * @param   timeout     Maximum iteration count
 * @return  1 if flag was set before timeout, 0 otherwise
 */
static uint8_t Display_WaitI2cFlag(uint32_t i2c_periph, i2c_flag_enum flag, uint32_t timeout)
{
    while (timeout--)
    {
        if (SET == i2c_flag_get(i2c_periph, flag))
        {
            return 1U;
        }
    }
    return 0U;
}

/**
 * @brief   Wait for the I2C STOP bit to clear.
 * @param   i2c_periph  I2C peripheral base
 * @param   timeout     Maximum iteration count
 * @return  1 if STOP cleared before timeout, 0 otherwise
 */
static uint8_t Display_WaitI2cStop(uint32_t i2c_periph, uint32_t timeout)
{
    while (timeout--)
    {
        if (0U == (I2C_CTL0(i2c_periph) & I2C_CTL0_STOP))
        {
            return 1U;
        }
    }
    return 0U;
}

/**
 * @brief   Poll the DMA Full-Transfer-Finish flag.
 * @param   dma_periph  DMA peripheral base
 * @param   channel     DMA channel
 * @param   timeout     Maximum iteration count
 * @return  1 if FTF was set before timeout, 0 otherwise
 */
static uint8_t Display_WaitDma(uint32_t dma_periph, dma_channel_enum channel, uint32_t timeout)
{
    while (timeout--)
    {
        if (SET == dma_flag_get(dma_periph, channel, DMA_FLAG_FTF))
        {
            return 1U;
        }
    }
    return 0U;
}

/**
 * @brief   Wait for ADDSEND or detect NACK (AERR) after addressing.
 * @param   timeout  Maximum iteration count
 * @return  1 if ADDSEND was set (ACK received), 0 on timeout or NACK
 */
static uint8_t Display_WaitAddrOrNack(uint32_t timeout)
{
    while (timeout--)
    {
        if (SET == i2c_flag_get(I2C0, I2C_FLAG_ADDSEND))
        {
            return 1U;
        }
        if (SET == i2c_flag_get(I2C0, I2C_FLAG_AERR))
        {
            i2c_flag_clear(I2C0, I2C_FLAG_AERR);
            return 0U;
        }
    }
    return 0U;
}

/** SSD1306 initialization command sequence */
uint8_t kInitCmdSeq[] = {
    0xAE,       /* display off */
    0xD5, 0x80, /* set display clock divide ratio / oscillator frequency */
    0xA8, 0x1F, /* set multiplex ratio */
    0xD3, 0x00, /* display offset */
    0x40,       /* set display start line */
    0x8d, 0x14, /* set charge pump */
    0xa1,       /* set segment re-map */
    0xc8,       /* set COM output scan direction */
    0xda, 0x00, /* set COM pins hardware configuration */
    0x81, 0x80, /* set contrast control */
    0xd9, 0x1f, /* set pre-charge period */
    0xdb, 0x40, /* set VCOM deselect level */
    0xa4,       /* set entire display on/off */
    0xaf,       /* set display on */
};

/**
 * @brief   Send a command byte to the SSD1306 via I2C DMA.
 * @param   cmd  Command byte
 */
void Display_WriteCmd(uint8_t cmd)
{
    uint32_t timeout = 10000;

    if (0U == m_DisplayReady)
    {
        return;
    }

    g_OledCmdBuf[0] = 0x00;
    g_OledCmdBuf[1] = cmd;

    /* If bus is busy, perform a bus recovery first */
    if (i2c_flag_get(I2C0, I2C_FLAG_I2CBSY))
    {
        I2cBus_Reset();
    }

    while (i2c_flag_get(I2C0, I2C_FLAG_I2CBSY) && (--timeout > 0))
    {
        delay_ms(1);
    }

    /* On timeout, reset once more and retry */
    if (timeout == 0)
    {
        I2cBus_Reset();
        timeout = 10000;
        while (i2c_flag_get(I2C0, I2C_FLAG_I2CBSY) && (--timeout > 0))
        {
            delay_ms(1);
        }
        if (timeout == 0)
        {
            return;
        }
    }

    i2c_start_on_bus(I2C0);
    if (!Display_WaitI2cFlag(I2C0, I2C_FLAG_SBSEND, DISPLAY_I2C_TIMEOUT))
    {
        I2cBus_Reset();
        m_DisplayReady = 0U;
        return;
    }

    i2c_master_addressing(I2C0, DISPLAY_I2C_ADDR_WR, I2C_TRANSMITTER);
    if (!Display_WaitAddrOrNack(DISPLAY_I2C_TIMEOUT))
    {
        i2c_stop_on_bus(I2C0);
        (void)Display_WaitI2cStop(I2C0, DISPLAY_I2C_TIMEOUT / 10U);
        I2cBus_Reset();
        m_DisplayReady = 0U;
        return;
    }

    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);
    if (!Display_WaitI2cFlag(I2C0, I2C_FLAG_TBE, DISPLAY_I2C_TIMEOUT))
    {
        I2cBus_Reset();
        m_DisplayReady = 0U;
        return;
    }

    dma_memory_address_config(DMA0, DMA_CH6, DMA_MEMORY_0, (uint32_t)g_OledCmdBuf);
    dma_transfer_number_config(DMA0, DMA_CH6, 2);
    i2c_dma_config(I2C0, I2C_DMA_ON);
    dma_channel_enable(DMA0, DMA_CH6);

    if (!Display_WaitDma(DMA0, DMA_CH6, DISPLAY_I2C_TIMEOUT))
    {
        dma_channel_disable(DMA0, DMA_CH6);
        i2c_dma_config(I2C0, I2C_DMA_OFF);
        I2cBus_Reset();
        m_DisplayReady = 0U;
        return;
    }

    dma_flag_clear(DMA0, DMA_CH6, DMA_FLAG_FTF);
    dma_channel_disable(DMA0, DMA_CH6);
    i2c_dma_config(I2C0, I2C_DMA_OFF);
    i2c_stop_on_bus(I2C0);

    if (!Display_WaitI2cStop(I2C0, DISPLAY_I2C_TIMEOUT))
    {
        I2cBus_Reset();
        m_DisplayReady = 0U;
        return;
    }
}

/**
 * @brief   Send a data byte to the SSD1306 via I2C DMA.
 * @param   data  Data byte
 */
void Display_WriteData(uint8_t data)
{
    uint32_t timeout = 10000;

    if (0U == m_DisplayReady)
    {
        return;
    }

    g_OledDataBuf[0] = 0x40;
    g_OledDataBuf[1] = data;

    /* If bus is busy, perform a bus recovery first */
    if (i2c_flag_get(I2C0, I2C_FLAG_I2CBSY))
    {
        I2cBus_Reset();
    }

    while (i2c_flag_get(I2C0, I2C_FLAG_I2CBSY) && (--timeout > 0))
    {
        delay_ms(1);
    }

    /* On timeout, reset once more and retry */
    if (timeout == 0)
    {
        I2cBus_Reset();
        timeout = 10000;
        while (i2c_flag_get(I2C0, I2C_FLAG_I2CBSY) && (--timeout > 0))
        {
            delay_ms(1);
        }
        if (timeout == 0)
        {
            return;
        }
    }

    i2c_start_on_bus(I2C0);
    if (!Display_WaitI2cFlag(I2C0, I2C_FLAG_SBSEND, DISPLAY_I2C_TIMEOUT))
    {
        I2cBus_Reset();
        m_DisplayReady = 0U;
        return;
    }

    i2c_master_addressing(I2C0, DISPLAY_I2C_ADDR_WR, I2C_TRANSMITTER);
    if (!Display_WaitAddrOrNack(DISPLAY_I2C_TIMEOUT))
    {
        i2c_stop_on_bus(I2C0);
        (void)Display_WaitI2cStop(I2C0, DISPLAY_I2C_TIMEOUT / 10U);
        I2cBus_Reset();
        m_DisplayReady = 0U;
        return;
    }

    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);
    if (!Display_WaitI2cFlag(I2C0, I2C_FLAG_TBE, DISPLAY_I2C_TIMEOUT))
    {
        I2cBus_Reset();
        m_DisplayReady = 0U;
        return;
    }

    dma_memory_address_config(DMA0, DMA_CH6, DMA_MEMORY_0, (uint32_t)g_OledDataBuf);
    dma_transfer_number_config(DMA0, DMA_CH6, 2);
    i2c_dma_config(I2C0, I2C_DMA_ON);
    dma_channel_enable(DMA0, DMA_CH6);

    if (!Display_WaitDma(DMA0, DMA_CH6, DISPLAY_I2C_TIMEOUT))
    {
        dma_channel_disable(DMA0, DMA_CH6);
        i2c_dma_config(I2C0, I2C_DMA_OFF);
        I2cBus_Reset();
        m_DisplayReady = 0U;
        return;
    }

    dma_flag_clear(DMA0, DMA_CH6, DMA_FLAG_FTF);
    dma_channel_disable(DMA0, DMA_CH6);
    i2c_dma_config(I2C0, I2C_DMA_OFF);
    i2c_stop_on_bus(I2C0);

    if (!Display_WaitI2cStop(I2C0, DISPLAY_I2C_TIMEOUT))
    {
        I2cBus_Reset();
        m_DisplayReady = 0U;
        return;
    }
}

/**
 * @brief   Display a monochrome bitmap region.
 * @param   x0    Start X coordinate
 * @param   y0    Start Y coordinate (page)
 * @param   x1    End X coordinate (exclusive)
 * @param   y1    End Y coordinate (exclusive)
 * @param   BMP   Bitmap data array
 */
void Display_ShowPic(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t BMP[])
{
    uint16_t i = 0;
    uint8_t x, y;
    for (y = y0; y < y1; y++)
    {
        Display_SetPos(x0, y);
        for (x = x0; x < x1; x++)
        {
            Display_WriteData(BMP[i++]);
        }
    }
}

/**
 * @brief   Display a 16x16 Chinese character.
 * @param   x   X coordinate (column)
 * @param   y   Y coordinate (page)
 * @param   no  Character index in Hzk[]
 */
void Display_ShowHanzi(uint8_t x, uint8_t y, uint8_t no)
{
    uint8_t t, adder = 0;
    Display_SetPos(x, y);
    for (t = 0; t < 16; t++)
    {
        Display_WriteData(Hzk[2 * no][t]);
        adder += 1;
    }
    Display_SetPos(x, y + 1);
    for (t = 0; t < 16; t++)
    {
        Display_WriteData(Hzk[2 * no + 1][t]);
        adder += 1;
    }
}

/**
 * @brief   Display a 32x32 Chinese character.
 * @param   x   X coordinate (column)
 * @param   y   Y coordinate (page)
 * @param   n   Character index in Hzb[]
 */
void Display_ShowHzbig(uint8_t x, uint8_t y, uint8_t n)
{
    uint8_t t, adder = 0;
    Display_SetPos(x, y);
    for (t = 0; t < 32; t++)
    {
        Display_WriteData(Hzb[4 * n][t]);
        adder += 1;
    }
    Display_SetPos(x, y + 1);
    for (t = 0; t < 32; t++)
    {
        Display_WriteData(Hzb[4 * n + 1][t]);
        adder += 1;
    }

    Display_SetPos(x, y + 2);
    for (t = 0; t < 32; t++)
    {
        Display_WriteData(Hzb[4 * n + 2][t]);
        adder += 1;
    }
    Display_SetPos(x, y + 3);
    for (t = 0; t < 32; t++)
    {
        Display_WriteData(Hzb[4 * n + 3][t]);
        adder += 1;
    }
}

/**
 * @brief   Display a floating-point number.
 * @param   x         X coordinate (column)
 * @param   y         Y coordinate (page)
 * @param   num       Number to display
 * @param   accuracy  Decimal places
 * @param   fontsize  Font size (8 = 6x8, 16 = 8x16)
 */
void Display_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t accuracy, uint8_t fontsize)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t t = 0;
    uint8_t temp = 0;
    uint16_t numel = 0;
    uint32_t integer = 0;
    float decimals = 0;

    if (num < 0)
    {
        Display_ShowChar(x, y, '-', fontsize);
        num = 0 - num;
        i++;
    }

    integer = (uint32_t)num;
    decimals = num - integer;

    if (integer)
    {
        numel = integer;

        while (numel)
        {
            numel /= 10;
            j++;
        }
        i += (j - 1);
        for (temp = 0; temp < j; temp++)
        {
            Display_ShowChar(x + 8 * (i - temp), y, integer % 10 + '0', fontsize);
            integer /= 10;
        }
    }
    else
    {
        Display_ShowChar(x + 8 * i, y, temp + '0', fontsize);
    }
    i++;
    if (accuracy)
    {
        Display_ShowChar(x + 8 * i, y, '.', fontsize);

        i++;
        for (t = 0; t < accuracy; t++)
        {
            decimals *= 10;
            temp = (uint8_t)decimals;
            Display_ShowChar(x + 8 * (i + t), y, temp + '0', fontsize);
            decimals -= temp;
        }
    }
}

/**
 * @brief   Compute power (base^exponent).
 * @param   a  Base
 * @param   n  Exponent
 * @return  a^n
 */
static uint32_t Display_Pow(uint8_t a, uint8_t n)
{
    uint32_t result = 1;
    while (n--)
    {
        result *= a;
    }
    return result;
}

/**
 * @brief   Display an unsigned integer.
 * @param   x         X coordinate (column)
 * @param   y         Y coordinate (page)
 * @param   num       Number to display
 * @param   length    Total digit width
 * @param   fontsize  Font size (8 = 6x8, 16 = 8x16)
 */
void Display_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t length, uint8_t fontsize)
{
    uint8_t t, temp;
    uint8_t enshow = 0;
    for (t = 0; t < length; t++)
    {
        temp = (num / Display_Pow(10, length - t - 1)) % 10;
        if (enshow == 0 && t < (length - 1))
        {
            if (temp == 0)
            {
                Display_ShowChar(x + (fontsize / 2) * t, y, ' ', fontsize);
                continue;
            }
            else
            {
                enshow = 1;
            }
        }
        Display_ShowChar(x + (fontsize / 2) * t, y, temp + '0', fontsize);
    }
}

/**
 * @brief   Display a null-terminated string.
 * @param   x         X coordinate (column)
 * @param   y         Y coordinate (page)
 * @param   ch        Pointer to string
 * @param   fontsize  Font size (8 = 6x8, 16 = 8x16)
 */
void Display_ShowStr(uint8_t x, uint8_t y, char *ch, uint8_t fontsize)
{
    uint8_t j = 0;
    while (ch[j] != '\0')
    {
        Display_ShowChar(x, y, ch[j], fontsize);
        x += 8;
        if (x > 120)
        {
            x = 0;
            y += 2;
        }
        j++;
    }
}

/**
 * @brief   Display a single ASCII character.
 * @param   x         X coordinate (column)
 * @param   y         Y coordinate (page)
 * @param   ch        ASCII character code
 * @param   fontsize  Font size (8 = 6x8, 16 = 8x16)
 */
void Display_ShowChar(uint8_t x, uint8_t y, uint8_t ch, uint8_t fontsize)
{
    uint8_t c = 0, i = 0;
    c = ch - ' ';

    if (x > 127)  /* beyond the right boundary */
    {
        x = 0;
        y++;
    }

    if (fontsize == 16)
    {
        Display_SetPos(x, y);
        for (i = 0; i < 8; i++)
        {
            Display_WriteData(F8X16[c * 16 + i]);
        }
        Display_SetPos(x, y + 1);
        for (i = 0; i < 8; i++)
        {
            Display_WriteData(F8X16[c * 16 + i + 8]);
        }
    }
    else
    {
        Display_SetPos(x, y);
        for (i = 0; i < 6; i++)
        {
            Display_WriteData(F6X8[c][i]);
        }
    }
}

/**
 * @brief   Fill the entire display with 0xFF (all pixels on).
 */
void Display_Fill(void)
{
    uint8_t i, j;
    for (i = 0; i < 4; i++)
    {
        Display_WriteCmd(0xb0 + i);
        Display_WriteCmd(0x00);
        Display_WriteCmd(0x10);
        for (j = 0; j < 128; j++)
        {
            Display_WriteData(0xFF);
        }
    }
}

/**
 * @brief   Set the cursor position.
 * @param   x  Column (0-127)
 * @param   y  Page (0-3 for 32-pixel height)
 */
void Display_SetPos(uint8_t x, uint8_t y)
{
    Display_WriteCmd(0xb0 + y);
    Display_WriteCmd(((x & 0xf0) >> 4) | 0x10);
    Display_WriteCmd((x & 0x0f) | 0x00);
}

/**
 * @brief   Clear the entire display (fill with 0x00).
 */
void Display_Clear(void)
{
    uint8_t i, n;
    for (i = 0; i < 4; i++)
    {
        Display_WriteCmd(0xb0 + i);
        Display_WriteCmd(0x00);
        Display_WriteCmd(0x10);
        for (n = 0; n < 128; n++)
        {
            Display_WriteData(0);
        }
    }
}

/**
 * @brief   Turn the display on (charge pump + display enable).
 */
void Display_On(void)
{
    Display_WriteCmd(0x8D);
    Display_WriteCmd(0x14);
    Display_WriteCmd(0xAF);
}

/**
 * @brief   Turn the display off.
 */
void Display_Off(void)
{
    Display_WriteCmd(0x8D);
    Display_WriteCmd(0x10);
    Display_WriteCmd(0xAF);
}

/**
 * @brief   Initialize the SSD1306 OLED display.
 *
 * Sends the full initialization command sequence, clears the screen,
 * and sets cursor to (0, 0).
 */
void Display_Init(void)
{
    delay_ms(100);
    m_DisplayReady = 1U;

    {
        uint8_t i;
        for (i = 0; i < sizeof(kInitCmdSeq); i++)
        {
            Display_WriteCmd(kInitCmdSeq[i]);
            if (0U == m_DisplayReady)
            {
                return;
            }
        }
    }

    Display_Clear();
    Display_SetPos(0, 0);
}
