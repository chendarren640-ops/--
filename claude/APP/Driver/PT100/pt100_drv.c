/**
 * 2026 CIMC APP — PT100 驱动实现
 *
 * 模拟 I2C (PB8/PB9) → GD30AD3340 (外部 16-bit ΔΣ ADC) → PT100 转换
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
 * 模拟 I2C 底层 (PB6=SCL, PB7=SDA, 开漏输出)
 * ================================================================ */
static void i2c_delay(void) {
    volatile uint32_t i; for(i=0;i<80;i++) __NOP();
}
static void scl_h(void) { gpio_bit_set(PT100_I2C_PORT, PT100_I2C_SCL); }
static void scl_l(void) { gpio_bit_reset(PT100_I2C_PORT, PT100_I2C_SCL); }
static void sda_h(void) { gpio_bit_set(PT100_I2C_PORT, PT100_I2C_SDA); }
static void sda_l(void) { gpio_bit_reset(PT100_I2C_PORT, PT100_I2C_SDA); }
static uint8_t sda_read(void) { return gpio_input_bit_get(PT100_I2C_PORT, PT100_I2C_SDA); }

static void i2c_start(void) {
    sda_h(); scl_h(); i2c_delay();
    sda_l(); i2c_delay(); scl_l(); i2c_delay();
}
static void i2c_stop(void) {
    sda_l(); i2c_delay();
    scl_h(); i2c_delay(); sda_h(); i2c_delay();
}
static uint8_t i2c_wait_ack(void) {
    uint16_t to=0;
    scl_l(); sda_h(); i2c_delay();
    scl_h(); i2c_delay();
    while(sda_read()) { if(++to>500){scl_l();return 1;} }
    scl_l(); i2c_delay(); return 0;
}
static void i2c_send_ack(void)   { scl_l(); sda_l(); i2c_delay(); scl_h(); i2c_delay(); scl_l(); sda_h(); i2c_delay(); }
static void i2c_send_nack(void)  { scl_l(); sda_h(); i2c_delay(); scl_h(); i2c_delay(); scl_l(); i2c_delay(); }

static void i2c_send_byte(uint8_t b) {
    for(uint8_t i=0;i<8;i++) {
        scl_l(); if(b&0x80)sda_h();else sda_l(); b<<=1; i2c_delay();
        scl_h(); i2c_delay(); scl_l(); i2c_delay();
    }
    sda_h();
}
static uint8_t i2c_read_byte(void) {
    uint8_t b=0; sda_h();
    for(uint8_t i=0;i<8;i++) {
        b<<=1; scl_l(); i2c_delay();
        scl_h(); i2c_delay();
        if(sda_read())b|=1;
        scl_l(); i2c_delay();
    }
    return b;
}

/* ================================================================
 * GD30AD3340 寄存器读写 (16-bit 寄存器)
 * ================================================================ */
static uint8_t ad3340_write_reg(uint8_t reg, uint16_t val) {
    uint8_t d[2]={(uint8_t)(val>>8),(uint8_t)(val&0xFF)};
    i2c_start();
    i2c_send_byte((uint8_t)(PT100_I2C_ADDR<<1));
    if(i2c_wait_ack()){i2c_stop();return 1;}
    i2c_send_byte(reg);
    if(i2c_wait_ack()){i2c_stop();return 1;}
    for(int i=0;i<2;i++){i2c_send_byte(d[i]);if(i2c_wait_ack()){i2c_stop();return 1;}}
    i2c_stop();return 0;
}

static uint8_t ad3340_read_reg(uint8_t reg, uint16_t *val) {
    uint8_t d[2];
    i2c_start();
    i2c_send_byte((uint8_t)(PT100_I2C_ADDR<<1));
    if(i2c_wait_ack()){i2c_stop();return 1;}
    i2c_send_byte(reg);
    if(i2c_wait_ack()){i2c_stop();return 1;}
    i2c_start();
    i2c_send_byte((uint8_t)((PT100_I2C_ADDR<<1)|0x01));
    if(i2c_wait_ack()){i2c_stop();return 1;}
    for(int i=0;i<2;i++){d[i]=i2c_read_byte();if(i<1)i2c_send_ack();else i2c_send_nack();}
    i2c_stop();
    *val=((uint16_t)d[0]<<8)|d[1];
    return 0;
}

/* ================================================================
 * 公开 API
 * ================================================================ */
void PT100_Init(void) {
    /* I2C 引脚初始化: PB6/PB7, 开漏输出, 上拉 */
    rcu_periph_clock_enable(RCU_GPIOB);
    gpio_mode_set(PT100_I2C_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, PT100_I2C_SCL|PT100_I2C_SDA);
    gpio_output_options_set(PT100_I2C_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, PT100_I2C_SCL|PT100_I2C_SDA);
    scl_h(); sda_h(); i2c_delay();

    /* 配置 GD30AD3340: AIN0 单端, ±4.096V, 100SPS */
    ad3340_write_reg(GD30AD3340_REG_CONFIG, GD30AD3340_CONFIG);
    delay_1ms(10);
}

uint8_t PT100_ReadRaw(int16_t *raw, float *voltage) {
    uint16_t conv, status;
    uint8_t ret;

    /* 启动单次转换 */
    ret = ad3340_write_reg(GD30AD3340_REG_CONFIG, GD30AD3340_CONFIG | GD30AD3340_OS_START);
    if(ret) return ret;

    /* 等转换完成 (轮询 OS 位, 最多 30ms) */
    for(uint32_t to=0;to<30;to++) {
        delay_1ms(1);
        ret = ad3340_read_reg(GD30AD3340_REG_CONFIG, &status);
        if(ret) return ret;
        if(!(status & GD30AD3340_OS_START)) break;
    }
    if(status & GD30AD3340_OS_START) return 0xEE; /* 超时 */

    /* 读结果 */
    ret = ad3340_read_reg(GD30AD3340_REG_CONVERSION, &conv);
    if(ret) return ret;

    *raw = (int16_t)conv;
    if(voltage) *voltage = ((float)(*raw) * GD30AD3340_FSR) / 32768.0f;
    return 0;
}

float PT100_VoltageToTemperature(float v_adc) {
    float v_pt100 = (v_adc - PT100_BIAS) / PT100_GAIN;
    if(v_pt100 < 0.0f) return PT100_ERROR;
    float r = v_pt100 / PT100_I_EXC;
    if(r <= 0.0f) return PT100_ERROR;

    /* 二次方程求解温度: R = R0*(1 + A*T + B*T^2) */
    float disc = PT100_A_COEFF*PT100_A_COEFF - 4.0f*PT100_B_COEFF*(1.0f - r/PT100_R0);
    if(disc < 0) return PT100_ERROR;
    return (-PT100_A_COEFF + sqrtf(disc)) / (2.0f*PT100_B_COEFF);
}

float PT100_ReadTemperature(void) {
    int16_t raw; float voltage;
    if(PT100_ReadRaw(&raw, &voltage) != 0) return PT100_ERROR;
    return PT100_VoltageToTemperature(voltage);
}
