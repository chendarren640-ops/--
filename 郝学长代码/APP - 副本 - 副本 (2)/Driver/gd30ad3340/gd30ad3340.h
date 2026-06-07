#ifndef __GD30AD3340_H
#define __GD30AD3340_H

#include "gd32f4xx.h"           /* GD32F4 系列 MCU 标准外设库头文件 */
#include <stdint.h>             /* 标准整型定义 */

/* 当前硬件使用的量程 */
/* 当前硬件采用的量程（建议 ±2.048V，分辨率更高） */
#define GD30AD3340_FSR_VOLTAGE     2.048f
#define GD30AD3340_DEFAULT_CONFIG  GD30AD3340_CONFIG_AIN0_SINGLE_2V048_100SPS


/*
    GD30AD3340 I2C 地址选择
*/
#define GD30AD3340_ADDR_GND        0x48
#define GD30AD3340_ADDR_VDD        0x49
#define GD30AD3340_ADDR_SDA        0x4A
#define GD30AD3340_ADDR_SCL        0x4B

/* 当前工程默认使用的 I2C 设备地址（ADDR 接 GND） */
#define GD30AD3340_ADDR            GD30AD3340_ADDR_GND

/*
    寄存器地址定义
*/
#define GD30AD3340_REG_CONVERSION  0x00
#define GD30AD3340_REG_CONFIG      0x01
#define GD30AD3340_REG_LO_THRESH   0x02
#define GD30AD3340_REG_HI_THRESH   0x03

/*
    配置寄存器位定义
*/
#define GD30AD3340_OS_START        0x8000   /* 写入1启动单次转换 */

/* 输入多路选择器 */
#define GD30AD3340_MUX_AIN0_AIN1   0x0000
#define GD30AD3340_MUX_AIN0_AIN3   0x1000
#define GD30AD3340_MUX_AIN1_AIN3   0x2000
#define GD30AD3340_MUX_AIN2_AIN3   0x3000
#define GD30AD3340_MUX_AIN0_GND    0x4000
#define GD30AD3340_MUX_AIN1_GND    0x5000
#define GD30AD3340_MUX_AIN2_GND    0x6000
#define GD30AD3340_MUX_AIN3_GND    0x7000

/* 可编程增益（满量程） */
#define GD30AD3340_PGA_6_144V      0x0000
#define GD30AD3340_PGA_4_096V      0x0200
#define GD30AD3340_PGA_2_048V      0x0400
#define GD30AD3340_PGA_1_024V      0x0600
#define GD30AD3340_PGA_0_512V      0x0800
#define GD30AD3340_PGA_0_256V      0x0A00
#define GD30AD3340_PGA_0_064V      0x0C00

/* 工作模式 */
#define GD30AD3340_MODE_CONTINUOUS 0x0000
#define GD30AD3340_MODE_SINGLE     0x0100

/* 数据速率 */
#define GD30AD3340_DR_6_25SPS      0x0000
#define GD30AD3340_DR_12_5SPS      0x0020
#define GD30AD3340_DR_25SPS        0x0040
#define GD30AD3340_DR_50SPS        0x0060
#define GD30AD3340_DR_100SPS       0x0080
#define GD30AD3340_DR_250SPS       0x00A0
#define GD30AD3340_DR_500SPS       0x00C0
#define GD30AD3340_DR_1000SPS      0x00E0

/* 比较器禁用 */
#define GD30AD3340_COMP_DISABLE    0x0003

/*
    预定义配置宏（★ 注意：这些配置字不包含 OS_START，启动转换时再添加）
*/
/* 默认配置：AIN0单端，±2.048V，单次，100SPS，比较器关闭 */
#define GD30AD3340_CONFIG_AIN0_SINGLE_2V048_100SPS        \
        (GD30AD3340_MUX_AIN0_GND |                        \
         GD30AD3340_PGA_2_048V |                          \
         GD30AD3340_MODE_SINGLE |                         \
         GD30AD3340_DR_100SPS |                           \
         GD30AD3340_COMP_DISABLE)

/* 扩展量程：AIN0单端，±4.096V，单次，100SPS，比较器关闭 */
#define GD30AD3340_CONFIG_AIN0_SINGLE_4V096_100SPS        \
        (GD30AD3340_MUX_AIN0_GND |                        \
         GD30AD3340_PGA_4_096V |                          \
         GD30AD3340_MODE_SINGLE |                         \
         GD30AD3340_DR_100SPS |                           \
         GD30AD3340_COMP_DISABLE)

/* 函数声明 */
void GD30AD3340_Init(void);
uint8_t GD30AD3340_WriteReg16(uint8_t reg, uint16_t value);
uint8_t GD30AD3340_ReadReg16(uint8_t reg, uint16_t *value);
uint8_t GD30AD3340_ReadRawSingle(uint16_t config, int16_t *raw);
uint8_t GD30AD3340_ReadVoltageSingle(uint16_t config, float fsr, float *voltage);
uint8_t GD30AD3340_ReadRawVoltageSingle(uint16_t config, float fsr, int16_t *raw, float *voltage);
uint8_t GD30AD3340_ReadAIN0Default(int16_t *raw, float *voltage);

#endif /* __GD30AD3340_H */

