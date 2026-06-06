/**
 * Bootloader — OLED 精简驱动 (SSD1306, 128x32, I2C)
 *
 * 从 APP/Driver/oled_drv.c 裁剪, 去除画点/线/圆/字符/汉字/图片功能,
 * 仅保留: 初始化 + 双行8pt ASCII字符串显示 + 清屏 + 刷新
 */
#include "oled_bl.h"
#include "systick_bl.h"

/* OLED 缓冲 (128x4页) */
static uint8_t OLED_GRAM[128][4];

/* I2C 引脚宏 */
#define SCL_P  GPIOB
#define SCL_B  GPIO_PIN_8
#define SDA_P  GPIOB
#define SDA_B  GPIO_PIN_9
#define SCL_H() gpio_bit_set(SCL_P,SCL_B)
#define SCL_L() gpio_bit_reset(SCL_P,SCL_B)
#define SDA_H() gpio_bit_set(SDA_P,SDA_B)
#define SDA_L() gpio_bit_reset(SDA_P,SDA_B)
#define SDA_IN() gpio_input_bit_get(SDA_P,SDA_B)

/* 8x8 ASCII 字库 (仅 ' ' ~ 'Z' 部分, 精简版) */
static const uint8_t font8x8[][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x00 */
    {0x00,0xFE,0xFE,0x00,0xFE,0xFE,0x00,0x00}, /* 0x01 '!' style */
};

static inline void i2c_dly(void)
{
    volatile uint16_t t = 120;
    while (t--);
}

static void i2c_start(void)
{
    gpio_mode_set(SDA_P, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, SDA_B);
    gpio_output_options_set(SDA_P, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, SDA_B);
    SDA_H(); SCL_H(); i2c_dly();
    SDA_L(); i2c_dly();
    SCL_L(); i2c_dly();
}

static void i2c_stop(void)
{
    gpio_mode_set(SDA_P, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, SDA_B);
    gpio_output_options_set(SDA_P, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, SDA_B);
    SDA_L(); SCL_H(); i2c_dly();
    SDA_H();
}

static uint8_t i2c_wait_ack(void)
{
    uint8_t to = 0;
    SDA_H(); i2c_dly(); SCL_H(); i2c_dly();
    gpio_mode_set(SDA_P, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, SDA_B);
    while (SDA_IN()) { if (++to > 254) { i2c_stop(); return 1; } }
    SCL_L(); i2c_dly();
    gpio_mode_set(SDA_P, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, SDA_B);
    gpio_output_options_set(SDA_P, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, SDA_B);
    return 0;
}

static void i2c_send(uint8_t d)
{
    for (uint8_t i = 0; i < 8; i++) {
        SCL_L();
        if (d & 0x80) SDA_H(); else SDA_L();
        i2c_dly(); SCL_H(); i2c_dly(); SCL_L();
        d <<= 1;
    }
}

static void oled_wr(uint8_t d, uint8_t cmd)
{
    i2c_start();
    i2c_send(0x78); i2c_wait_ack();
    i2c_send(cmd ? 0x40 : 0x00); i2c_wait_ack();
    i2c_send(d); i2c_wait_ack();
    i2c_stop();
}

void OLED_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    gpio_mode_set(SCL_P, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, SCL_B);
    gpio_output_options_set(SCL_P, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, SCL_B);
    gpio_mode_set(SDA_P, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, SDA_B);
    gpio_output_options_set(SDA_P, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, SDA_B);
    SCL_H(); SDA_H();
    delay_1ms(500);

    /* SSD1306 初始化序列 */
    uint8_t init_seq[] = {
        0xAE,0x00,0x10,0x40,0x81,0xCF,0xA1,0xC8,
        0xA6,0xA8,0x1F,0xD3,0x00,0xD5,0x80,0xD9,
        0xF1,0xDA,0x00,0xDB,0x40,0x20,0x02,0x8D,
        0x14,0xA4,0xA6,0xAF
    };
    for (uint8_t i = 0; i < sizeof(init_seq); i++)
        oled_wr(init_seq[i], 0);
    OLED_Clear();
}

void OLED_Clear(void)
{
    for (uint8_t p = 0; p < 4; p++)
        for (uint8_t c = 0; c < 128; c++)
            OLED_GRAM[c][p] = 0;
    OLED_Refresh();
}

void OLED_Refresh(void)
{
    for (uint8_t p = 0; p < 4; p++) {
        oled_wr(0xB0 + p, 0);
        oled_wr(0x00, 0); oled_wr(0x10, 0);
        for (uint8_t c = 0; c < 128; c++)
            oled_wr(OLED_GRAM[c][p], 1);
    }
}

/* 在 OLED_GRAM 中显示 8x8 ASCII 字符串 (精简字库) */
void OLED_ShowLine1(uint8_t *str) {
    uint8_t x = 0, page = 0;
    while (*str && page < 1) {
        if (x > 120) { x = 0; page++; }
        if (*str >= ' ' && *str <= 'Z') {
            for (uint8_t m = 0; m < 8; m++) {
                if (x + m < 128)
                    OLED_GRAM[x + m][page] = font8x8[*str - ' '][m];
            }
        }
        x += 8; str++;
    }
    OLED_Refresh();
}

void OLED_ShowLine2(uint8_t *str) {
    uint8_t x = 0, page = 2;
    while (*str && page < 3) {
        if (x > 120) { x = 0; page++; }
        if (*str >= ' ' && *str <= 'Z') {
            for (uint8_t m = 0; m < 8; m++) {
                if (x + m < 128)
                    OLED_GRAM[x + m][page] = font8x8[*str - ' '][m];
            }
        }
        x += 8; str++;
    }
    OLED_Refresh();
}
