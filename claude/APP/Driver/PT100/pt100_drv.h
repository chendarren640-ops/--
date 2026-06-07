/**
 * 2026 CIMC APP — PT100 驱动 (外部 ADC GD30AD3340 via I2C)
 *
 * 硬件连接:
 *   I2C SCL → PB6
 *   I2C SDA → PB7
 *   GD30AD3340 I2C 地址: 0x48 (ADDR 引脚接 GND)
 *   PT100 恒流源: 0.448mA, 仪表放大器增益: 1.0, 偏置: 0V
 *
 * 注意: 此 I2C 与 OLED 的 I2C (PB8/PB9) 是独立的两路, 互不冲突!
 */

#ifndef __PT100_DRV_H
#define __PT100_DRV_H
#include "main.h"

/* I2C 引脚定义 (PB8=SCL, PB9=SDA) — 与 OLED 共用 I2C 总线 */
#define PT100_I2C_PORT    GPIOB
#define PT100_I2C_SCL     GPIO_PIN_8
#define PT100_I2C_SDA     GPIO_PIN_9
#define PT100_I2C_ADDR    0x48   /* GD30AD3340 7-bit I2C 地址 */

/* GD30AD3340 寄存器 */
#define GD30AD3340_REG_CONVERSION  0x00
#define GD30AD3340_REG_CONFIG      0x01

/* GD30AD3340 配置 */
#define GD30AD3340_OS_START        0x8000
#define GD30AD3340_MUX_AIN0_GND    0x4000
#define GD30AD3340_PGA_4_096V      0x0200
#define GD30AD3340_MODE_SINGLE     0x0100
#define GD30AD3340_DR_100SPS       0x0080
#define GD30AD3340_COMP_DISABLE    0x0003

#define GD30AD3340_CONFIG  (GD30AD3340_MUX_AIN0_GND | \
                            GD30AD3340_PGA_4_096V  | \
                            GD30AD3340_MODE_SINGLE | \
                            GD30AD3340_DR_100SPS   | \
                            GD30AD3340_COMP_DISABLE)
#define GD30AD3340_FSR     4.096f

/* PT100 参数 */
#define PT100_R0         100.0f
#define PT100_A_COEFF    3.9083e-3f
#define PT100_B_COEFF   -5.775e-7f
#define PT100_I_EXC      0.000448f   /* 恒流源 0.448mA */
#define PT100_GAIN       1.0f        /* 放大器增益 */
#define PT100_BIAS       0.0f        /* 偏置电压 */
#define PT100_ERROR      (-273.15f)

/* 函数声明 */
void  PT100_Init(void);
float PT100_ReadTemperature(void);       /* 一次完整读数: 电压→电阻→温度 */
uint8_t PT100_ReadRaw(int16_t *raw, float *voltage);  /* 读原始值 */
float PT100_VoltageToTemperature(float voltage);       /* 电压直接转温度 */

#endif
