/**
 * 2026 CIMC APP — PT100 驱动实现
 *
 * SPI0 (PA5/PA6/PA7) → GD30AD3344 (外部 24-bit ΔΣ ADC) → PT100 转换
 */
#include "pt100_drv.h"

/* 本地 sqrtf — 不依赖 math.h / libm (兼容 Keil Microlib) */
static float local_sqrtf(float x) {
    if (x <= 0.0f) return 0.0f;
    float val = x, last;
    do { last = val; val = (val + x / val) * 0.5f; } while (val < last);
    return last;
}
#define sqrtf(x) local_sqrtf(x)

/* ================================================================
 * SPI 底层 (硬件 SPI, 轮询模式)
 * ================================================================ */
static void spi_cs_low(void)  { gpio_bit_reset(GD30_CS_PORT, GD30_CS_PIN); }
static void spi_cs_high(void) { gpio_bit_set(GD30_CS_PORT, GD30_CS_PIN); }

static uint16_t spi_transfer16(uint16_t tx_data) {
    while (RESET == spi_i2s_flag_get(GD30_SPI, SPI_FLAG_TBE));
    spi_i2s_data_transmit(GD30_SPI, tx_data);
    while (RESET == spi_i2s_flag_get(GD30_SPI, SPI_FLAG_RBNE));
    return spi_i2s_data_receive(GD30_SPI);
}

static uint8_t spi_transfer8(uint8_t tx_data) {
    while (RESET == spi_i2s_flag_get(GD30_SPI, SPI_FLAG_TBE));
    spi_i2s_data_transmit(GD30_SPI, tx_data);
    while (RESET == spi_i2s_flag_get(GD30_SPI, SPI_FLAG_RBNE));
    return (uint8_t)spi_i2s_data_receive(GD30_SPI);
}

/* ================================================================
 * GD30AD3344 寄存器读写 (16-bit 配置寄存器, 24-bit AD 结果)
 * ================================================================ */
static uint16_t gd30_read_config(void) {
    uint16_t val;
    spi_cs_low();
    spi_transfer16(GD30_CMD_RREG | GD30_REG_CONFIG);
    val = spi_transfer16(0x0000);
    spi_cs_high();
    return val;
}

static void gd30_write_config(uint16_t cfg) {
    spi_cs_low();
    spi_transfer16(GD30_CMD_WREG | GD30_REG_CONFIG);
    spi_transfer16(cfg);
    spi_cs_high();
}

static int32_t gd30_read_data(void) {
    int32_t result;
    uint8_t b2, b1, b0;

    spi_cs_low();
    spi_transfer16(GD30_CMD_RDATA);
    /* 读 24-bit 转换结果 (大端序, 有符号) */
    b2 = spi_transfer8(0x00);
    b1 = spi_transfer8(0x00);
    b0 = spi_transfer8(0x00);
    spi_cs_high();

    result = ((int32_t)(int8_t)b2 << 16) | ((uint16_t)b1 << 8) | b0;
    return result;
}

/* ================================================================
 * 公开 API
 * ================================================================ */
void PT100_Init(void) {
    /* SPI0 引脚: PA5=SCK, PA6=MISO, PA7=MOSI (AF5) */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_SPI0);

    gpio_af_set(GPIOA, GPIO_AF_5, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_5 | GPIO_PIN_7);

    /* CS 引脚: PE8, 推挽输出, 默认高 (未选中) */
    rcu_periph_clock_enable(RCU_GPIOE);
    gpio_mode_set(GD30_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GD30_CS_PIN);
    gpio_output_options_set(GD30_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GD30_CS_PIN);
    spi_cs_high();

    /* SPI0 配置: Mode 1 (CPOL=0, CPHA=1), 16-bit 帧, 预分频 256 */
    spi_parameter_struct spi_cfg;
    spi_struct_para_init(&spi_cfg);
    spi_cfg.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_cfg.device_mode          = SPI_MASTER;
    spi_cfg.frame_size           = SPI_FRAMESIZE_16BIT;
    spi_cfg.clock_polarity_phase = SPI_CK_PL_LOW_PH_2EDGE;  /* Mode 1 */
    spi_cfg.nss                  = SPI_NSS_SOFT;
    spi_cfg.prescale             = SPI_PSC_256;
    spi_cfg.endian               = SPI_ENDIAN_MSB;
    spi_init(GD30_SPI, &spi_cfg);
    spi_enable(GD30_SPI);

    /* 配置 GD30AD3344: AIN0 对 GND, PGA=±4.096V, 单次转换, 100SPS */
    gd30_write_config(GD30_CONFIG_DEFAULT);
    delay_1ms(10);
}

uint8_t PT100_ReadRaw(int32_t *raw, float *voltage) {
    uint16_t status;

    /* 启动单次转换 */
    gd30_write_config(GD30_CONFIG_DEFAULT | GD30_CONFIG_OS_START);

    /* 等转换完成 (轮询 OS 位, 最多 30ms) */
    for (uint32_t to = 0; to < 30; to++) {
        delay_1ms(1);
        status = gd30_read_config();
        if (!(status & GD30_CONFIG_OS_START)) break;
    }
    if (status & GD30_CONFIG_OS_START) return 0xEE; /* 超时 */

    /* 读 24-bit 结果 */
    *raw = gd30_read_data();

    /* 24-bit 有符号 → 电压 (FSR = ±4.096V, 2^23 = 8388608) */
    if (voltage) *voltage = ((float)(*raw) * GD30_FSR) / 8388608.0f;
    return 0;
}

float PT100_VoltageToTemperature(float v_adc) {
    float v_pt100 = (v_adc - PT100_BIAS) / PT100_GAIN;
    if (v_pt100 < 0.0f) return PT100_ERROR;
    float r = v_pt100 / PT100_I_EXC;
    if (r <= 0.0f) return PT100_ERROR;

    /* 二次方程求解温度: R = R0*(1 + A*T + B*T^2) */
    float disc = PT100_A_COEFF * PT100_A_COEFF - 4.0f * PT100_B_COEFF * (1.0f - r / PT100_R0);
    if (disc < 0) return PT100_ERROR;
    return (-PT100_A_COEFF + sqrtf(disc)) / (2.0f * PT100_B_COEFF);
}

float PT100_ReadTemperature(void) {
    int32_t raw; float voltage;
    if (PT100_ReadRaw(&raw, &voltage) != 0) return PT100_ERROR;
    return PT100_VoltageToTemperature(voltage);
}
