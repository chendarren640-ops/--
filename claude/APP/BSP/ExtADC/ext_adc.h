/* SPDX-License-Identifier: MIT */

/**
 * @file    ext_adc.h
 * @brief   GD30AD3344 24-bit delta-sigma ADC driver — SPI DMA interface.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "board_defs.h"

/** GD30AD3344 configuration register bit-field overlay */
typedef struct
{
    uint16_t SS         : 1;
    uint16_t MUX        : 3;
    uint16_t PGA        : 3;
    uint16_t MODE       : 1;
    uint16_t DR         : 3;
    uint16_t RESERVED_1 : 1;
    uint16_t PULL_UP_EN : 1;
    uint16_t NOP        : 2;
    uint16_t RESERVED   : 1;
} ExtAdcReg_s;

extern ExtAdcReg_s g_ExtAdcCfg;

/** One-shot conversion control */
typedef enum
{
    EXTADC_OS_DISABLE        = 0,
    EXTADC_OS_SINGLE_CONVERT = 1,
} ExtAdcOs_e;

/** Input channel (MUX setting) */
typedef enum
{
    EXTADC_CH_AIN0_AIN1      = 0, /**< AINP=AIN0, AINN=AIN1 */
    EXTADC_CH_AIN0_AIN3      = 1, /**< AINP=AIN0, AINN=AIN3 */
    EXTADC_CH_AIN1_AIN3      = 2, /**< AINP=AIN1, AINN=AIN3 */
    EXTADC_CH_AIN2_AIN3      = 3, /**< AINP=AIN2, AINN=AIN3 */
    EXTADC_CH_AIN0_GND       = 4, /**< AINP=AIN0, AINN=GND  */
    EXTADC_CH_AIN1_GND       = 5, /**< AINP=AIN1, AINN=GND  */
    EXTADC_CH_AIN2_GND       = 6, /**< AINP=AIN2, AINN=GND  */
    EXTADC_CH_AIN3_GND       = 7, /**< AINP=AIN3, AINN=GND  */
} ExtAdcCh_e;

/** PGA gain / full-scale range */
typedef enum
{
    EXTADC_PGA_6V144         = 0,
    EXTADC_PGA_4V096         = 1,
    EXTADC_PGA_2V048         = 2,
    EXTADC_PGA_1V024         = 3,
    EXTADC_PGA_0V512         = 4,
    EXTADC_PGA_0V256         = 5,
    EXTADC_PGA_0V064         = 6,
    EXTADC_PGA_RESERVED      = 7,
} ExtAdcPga_e;

/** Conversion mode */
typedef enum
{
    EXTADC_MODE_CONTINUOUS   = 0,
    EXTADC_MODE_SINGLE_SHOT  = 1,
} ExtAdcMode_e;

/** Data rate */
typedef enum
{
    EXTADC_DR_6_25SPS        = 0,
    EXTADC_DR_12_5SPS        = 1,
    EXTADC_DR_25SPS          = 2,
    EXTADC_DR_50SPS          = 3,
    EXTADC_DR_100SPS         = 4,
    EXTADC_DR_250SPS         = 5,
    EXTADC_DR_500SPS         = 6,
    EXTADC_DR_1000SPS        = 7,
} ExtAdcDr_e;

/** Internal pull-up on DOUT */
typedef enum
{
    EXTADC_PULLUP_DISABLE    = 0,
    EXTADC_PULLUP_ENABLE     = 1,
} ExtAdcPullUp_e;

/** NOP (no-operation) field values */
typedef enum
{
    EXTADC_NOP_INVALID   = 0,
    EXTADC_NOP_VALID_UPDATE  = 1,
    EXTADC_NOP_VALID_NO_UPD  = 2,
    EXTADC_NOP_INVALID2  = 3,
} ExtAdcNop_e;

/** Build the register word from g_ExtAdcCfg bit fields */
#define EXTADC_REG_VAL    ((uint16_t)((g_ExtAdcCfg.SS         << 15) | \
                                      (g_ExtAdcCfg.MUX        << 12) | \
                                      (g_ExtAdcCfg.PGA        <<  9) | \
                                      (g_ExtAdcCfg.MODE       <<  8) | \
                                      (g_ExtAdcCfg.DR         <<  5) | \
                                      (g_ExtAdcCfg.RESERVED_1 <<  4) | \
                                      (g_ExtAdcCfg.PULL_UP_EN <<  3) | \
                                      (g_ExtAdcCfg.NOP        <<  1) | \
                                      (g_ExtAdcCfg.RESERVED   <<  0)))

/* External reference register commands */
#define EXTADC_EXTREG_WRITE_CMD     0x8100U
#define EXTADC_EXTREG_READ_CMD      0x8106U
#define EXTADC_EXTREF_REG_ADDR      0x0014U
#define EXTADC_EXTPROC_REG_ADDR     0x0012U
#define EXTADC_EXTPROC_UNLOCK       0xACCAU
#define EXTADC_EXTREF_EN_BIT        0x0040U

extern ExtAdcReg_s g_ExtAdcCfg;
extern uint16_t g_ExtAdcRawCode;

/**
 * @brief   Initialize the GD30AD3344 with default configuration.
 */
void ExtAdc_Init(void);

/**
 * @brief   Read the current configuration register value.
 * @return  Raw configuration register value
 */
uint16_t ExtAdc_ReadCfg(void);

/**
 * @brief   Enable the external reference register bit.
 * @return  Register value read back after write
 */
uint16_t ExtAdc_ConfigRef(void);

/**
 * @brief   Read one ADC channel and convert the raw code to voltage.
 * @param   CH   ADC input channel
 * @param   Ref  PGA full-scale range
 * @return  Converted voltage in volts
 */
float ExtAdc_Read(ExtAdcCh_e CH, ExtAdcPga_e Ref);

#ifdef __cplusplus
}
#endif
