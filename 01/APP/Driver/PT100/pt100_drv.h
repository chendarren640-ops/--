/**
 * 2026 CIMC APP — PT100 驱动 (外部 ADC GD30AD3344 via SPI)
 *
 * 硬件连接:
 *   SPI0 SCK  → PA5
 *   SPI0 MISO → PA6
 *   SPI0 MOSI → PA7
 *   CS        → PE8
 *   PT100 恒流源: 0.448mA, 仪表放大器增益: 1.0, 偏置: 0V
 *
 * 注意: GD30AD3344 为 24-bit ΔΣ ADC, 通过 SPI 通信
 */

#ifndef __PT100_DRV_H
#define __PT100_DRV_H
#include "main.h"

/* SPI 引脚定义 */
#define GD30_SPI           SPI0
#define GD30_CS_PORT       GPIOE
#define GD30_CS_PIN        GPIO_PIN_8

/* GD30AD3344 SPI 命令 */
#define GD30_CMD_WREG      0x4000   /* 写寄存器 */
#define GD30_CMD_RREG      0x2000   /* 读寄存器 */
#define GD30_CMD_RDATA     0x1000   /* 读数据 */
#define GD30_REG_CONFIG    0x0001   /* 配置寄存器地址 */

/* GD30AD3344 配置位 (16-bit 配置寄存器) */
#define GD30_CONFIG_OS_START        0x8000
#define GD30_CONFIG_MUX_AIN0_GND    0x4000
#define GD30_CONFIG_PGA_4_096V      0x0200
#define GD30_CONFIG_MODE_SINGLE     0x0100
#define GD30_CONFIG_DR_100SPS       0x0080
#define GD30_CONFIG_COMP_DISABLE    0x0003

#define GD30_CONFIG_DEFAULT  (GD30_CONFIG_MUX_AIN0_GND | \
                              GD30_CONFIG_PGA_4_096V  | \
                              GD30_CONFIG_MODE_SINGLE | \
                              GD30_CONFIG_DR_100SPS   | \
                              GD30_CONFIG_COMP_DISABLE)
#define GD30_FSR     4.096f

/* PT100 参数 */
#define PT100_R0         100.0f
#define PT100_A_COEFF    3.9083e-3f
#define PT100_B_COEFF   -5.775e-7f
#define PT100_I_EXC      0.000448f   /* 恒流源 0.448mA */
#define PT100_GAIN       1.0f        /* 放大器增益 */
#define PT100_BIAS       0.0f        /* 偏置电压 */
#define PT100_ERROR      (-273.15f)

/* 函数声明 */
void    PT100_Init(void);
float   PT100_ReadTemperature(void);                          /* 一次完整读数: 电压→电阻→温度 */
uint8_t PT100_ReadRaw(int32_t *raw, float *voltage);          /* 读 24-bit 原始值 */
float   PT100_VoltageToTemperature(float voltage);            /* 电压直接转温度 */

#endif
