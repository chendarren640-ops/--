/* SPDX-License-Identifier: MIT */

/**
 * @file    board_defs.h
 * @brief   Central BSP header — pin-map, peripheral macros and HAL setup
 *          declarations for the GD32F470VET6 board.
 */

#pragma once

#include "gd32f4xx.h"
#include "gd32f4xx_dma.h"
#include "systick.h"

#include "ebtn.h"
#include "oled.h"
#include "gd25qxx.h"
#include "gd30ad3344.h"

#include "flash_fs_app.h"
#include "led_app.h"
#include "adc_app.h"
#include "oled_app.h"
#include "usart_app.h"
#include "rtc_app.h"
#include "btn_app.h"
#include "scheduler.h"
#include "ota_uart.h"
#include "rs485_phy.h"
#include "cimc_app.h"

#include "perf_counter.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************/
/* LED IO map
 * LED_SYS  -> PD8
 * LED_SMP  -> PD9
 * LED_AUX3 -> PD10
 * LED_AUX4 -> PD11
 * LED_AUX5 -> PD12
 * LED_AUX6 -> PD13
 */
#define LED_PORT              GPIOD
#define LED_CLK_PORT          RCU_GPIOD

#define LED_SYS_PIN           GPIO_PIN_8     /**< PD8  system status LED */
#define LED_SMP_PIN           GPIO_PIN_9     /**< PD9  sample status LED */
#define LED_AUX3_PIN          GPIO_PIN_10    /**< PD10 auxiliary LED 3 */
#define LED_AUX4_PIN          GPIO_PIN_11    /**< PD11 auxiliary LED 4 */
#define LED_AUX5_PIN          GPIO_PIN_12    /**< PD12 auxiliary LED 5 */
#define LED_AUX6_PIN          GPIO_PIN_13    /**< PD13 auxiliary LED 6 */

/** LED active level: 1 = active-high, 0 = active-low */
#define LED_ACTIVE_HIGH       0

#if LED_ACTIVE_HIGH
#define LED_WRITE(pin, on) \
    do \
    { \
        if (on) \
            GPIO_BOP(LED_PORT) = (pin); \
        else \
            GPIO_BC(LED_PORT) = (pin); \
    } \
    while (0)
#else
#define LED_WRITE(pin, on) \
    do \
    { \
        if (on) \
            GPIO_BC(LED_PORT) = (pin); \
        else \
            GPIO_BOP(LED_PORT) = (pin); \
    } \
    while (0)
#endif

#define LED_SYS_SET(x)        do { LED_WRITE(LED_SYS_PIN,  (x)); } while (0)
#define LED_SMP_SET(x)        do { LED_WRITE(LED_SMP_PIN,  (x)); } while (0)
#define LED_AUX3_SET(x)       do { LED_WRITE(LED_AUX3_PIN, (x)); } while (0)
#define LED_AUX4_SET(x)       do { LED_WRITE(LED_AUX4_PIN, (x)); } while (0)
#define LED_AUX5_SET(x)       do { LED_WRITE(LED_AUX5_PIN, (x)); } while (0)
#define LED_AUX6_SET(x)       do { LED_WRITE(LED_AUX6_PIN, (x)); } while (0)

#define LED_SYS_TOGGLE        do { GPIO_TG(LED_PORT) = LED_SYS_PIN;  } while (0)
#define LED_SMP_TOGGLE        do { GPIO_TG(LED_PORT) = LED_SMP_PIN;  } while (0)
#define LED_AUX3_TOGGLE       do { GPIO_TG(LED_PORT) = LED_AUX3_PIN; } while (0)
#define LED_AUX4_TOGGLE       do { GPIO_TG(LED_PORT) = LED_AUX4_PIN; } while (0)
#define LED_AUX5_TOGGLE       do { GPIO_TG(LED_PORT) = LED_AUX5_PIN; } while (0)
#define LED_AUX6_TOGGLE       do { GPIO_TG(LED_PORT) = LED_AUX6_PIN; } while (0)

#define LED_SYS_ON            do { LED_WRITE(LED_SYS_PIN,  1); } while (0)
#define LED_SMP_ON            do { LED_WRITE(LED_SMP_PIN,  1); } while (0)
#define LED_AUX3_ON           do { LED_WRITE(LED_AUX3_PIN, 1); } while (0)
#define LED_AUX4_ON           do { LED_WRITE(LED_AUX4_PIN, 1); } while (0)
#define LED_AUX5_ON           do { LED_WRITE(LED_AUX5_PIN, 1); } while (0)
#define LED_AUX6_ON           do { LED_WRITE(LED_AUX6_PIN, 1); } while (0)

#define LED_SYS_OFF           do { LED_WRITE(LED_SYS_PIN,  0); } while (0)
#define LED_SMP_OFF           do { LED_WRITE(LED_SMP_PIN,  0); } while (0)
#define LED_AUX3_OFF          do { LED_WRITE(LED_AUX3_PIN, 0); } while (0)
#define LED_AUX4_OFF          do { LED_WRITE(LED_AUX4_PIN, 0); } while (0)
#define LED_AUX5_OFF          do { LED_WRITE(LED_AUX5_PIN, 0); } while (0)
#define LED_AUX6_OFF          do { LED_WRITE(LED_AUX6_PIN, 0); } while (0)

/** Setup all LED GPIOs */
void HalLed_Setup(void);

/******************************************************************************/
/* Button IO map
 * BTN1 -> PE15
 * BTN2 -> PE13
 * BTN3 -> PE11
 * BTN4 -> PE9
 * BTN5 -> PE7
 * BTN6 -> PB0
 * BTN_WAKE -> PA0, wakeup key
 */
#define BTN_PORT_E            GPIOE
#define BTN_PORT_B            GPIOB
#define BTN_PORT_A            GPIOA
#define BTN_CLK_PORT_E        RCU_GPIOE
#define BTN_CLK_PORT_B        RCU_GPIOB
#define BTN_CLK_PORT_A        RCU_GPIOA

#define BTN1_PIN              GPIO_PIN_15    /**< PE15 button 1 */
#define BTN2_PIN              GPIO_PIN_13    /**< PE13 button 2 */
#define BTN3_PIN              GPIO_PIN_11    /**< PE11 button 3 */
#define BTN4_PIN              GPIO_PIN_9     /**< PE9  button 4 */
#define BTN5_PIN              GPIO_PIN_7     /**< PE7  button 5 */
#define BTN6_PIN              GPIO_PIN_0     /**< PB0  button 6 */
#define BTN_WAKE_PIN          GPIO_PIN_0     /**< PA0  wakeup button */

#define BTN1_READ             gpio_input_bit_get(BTN_PORT_E, BTN1_PIN)
#define BTN2_READ             gpio_input_bit_get(BTN_PORT_E, BTN2_PIN)
#define BTN3_READ             gpio_input_bit_get(BTN_PORT_E, BTN3_PIN)
#define BTN4_READ             gpio_input_bit_get(BTN_PORT_E, BTN4_PIN)
#define BTN5_READ             gpio_input_bit_get(BTN_PORT_E, BTN5_PIN)
#define BTN6_READ             gpio_input_bit_get(BTN_PORT_B, BTN6_PIN)
#define BTN_WAKE_READ         gpio_input_bit_get(BTN_PORT_A, BTN_WAKE_PIN)

/** Setup all button GPIOs with EXTI */
void HalBtn_Setup(void);

/** Configure the wakeup key EXTI line for deep-sleep exit */
void HalWakeup_Setup(void);

/** Enter deep-sleep (standby) mode */
void HalPwr_DeepSleep(void);

/** Enter deep-sleep and schedule RTC wakeup after @p seconds */
void HalPwr_DeepSleepRtc(uint16_t seconds);

/******************************************************************************/
/* OLED (I2C0 on PB8/PB9) */
#define OLED_I2C_OWN_ADDR     0x72
#define OLED_I2C_SLV_ADDR     0x82
#define OLED_I2C_DATA_ADDR    ((uint32_t)&I2C_DATA(I2C0))

#define OLED_I2C_SCL_PORT     GPIOB
#define OLED_I2C_SDA_PORT     GPIOB
#define OLED_I2C_PORT_RCU     RCU_GPIOB
#define OLED_I2C_SCL_PIN      GPIO_PIN_8
#define OLED_I2C_SDA_PIN      GPIO_PIN_9

/** Setup OLED over I2C0 */
void HalOled_Setup(void);

/******************************************************************************/
/* External SPI Flash (GD25Qxx) on SPI1, PB12-PB15 */
#define EXT_FLASH_SPI_PORT      GPIOB
#define EXT_FLASH_SPI_PORT_RCU  RCU_GPIOB

#define EXT_FLASH_CS_PIN        GPIO_PIN_12
#define EXT_FLASH_SPI_SCK       GPIO_PIN_13
#define EXT_FLASH_SPI_MISO      GPIO_PIN_14
#define EXT_FLASH_SPI_MOSI      GPIO_PIN_15

/** Setup external SPI Flash */
void HalExtFlash_Setup(void);

/******************************************************************************/
/* External ADC (GD30AD3344) on SPI0, PA5-PA7, PE8 CS */

/*
 * SPI_MODE_0  SPI_CK_PL_LOW_PH_1EDGE
 * SPI_MODE_1  SPI_CK_PL_LOW_PH_2EDGE
 * SPI_MODE_2  SPI_CK_PL_HIGH_PH_1EDGE
 * SPI_MODE_3  SPI_CK_PL_HIGH_PH_2EDGE
 */
#define SPI_MODE_0       SPI_CK_PL_LOW_PH_1EDGE
#define SPI_MODE_1       SPI_CK_PL_LOW_PH_2EDGE
#define SPI_MODE_2       SPI_CK_PL_HIGH_PH_1EDGE
#define SPI_MODE_3       SPI_CK_PL_HIGH_PH_2EDGE

#define EXT_ADC_SPI_MODE         SPI_MODE_1

#define EXT_ADC_SPI              SPI0
#define EXT_ADC_DMA              DMA1
#define EXT_ADC_SPI_TX_DMA_CH    DMA_CH5
#define EXT_ADC_SPI_RX_DMA_CH    DMA_CH0
#define EXT_ADC_SPI_DMA_SUBPERI  DMA_SUBPERI3

#define EXT_ADC_SPI_DMA_RCU      RCU_DMA1
#define EXT_ADC_SPI_RCU          RCU_SPI0

#define EXT_ADC_SPI_PORT         GPIOA
#define EXT_ADC_SPI_PORT_RCU     RCU_GPIOA
#define EXT_ADC_SPI_SCK          GPIO_PIN_5
#define EXT_ADC_SPI_MISO         GPIO_PIN_6
#define EXT_ADC_SPI_MOSI         GPIO_PIN_7

#define EXT_ADC_CS_PORT          GPIOE
#define EXT_ADC_CS_PORT_RCU      RCU_GPIOE
#define EXT_ADC_CS_PIN           GPIO_PIN_8

#define EXT_ADC_CS_LOW()         gpio_bit_reset(EXT_ADC_CS_PORT, EXT_ADC_CS_PIN)
#define EXT_ADC_CS_HIGH()        gpio_bit_set  (EXT_ADC_CS_PORT, EXT_ADC_CS_PIN)

/** Setup external ADC (GD30AD3344) over SPI */
void HalExtAdc_Setup(void);

/******************************************************************************/
/* UART — RS485 / DEBUG / OTA sharing USART1 (PA2/PA3), direction PA1 */

/** RS485 direction control: 1 = transmit, 0 = receive */
#define RS485_DIR_PORT          GPIOA
#define RS485_DIR_PORT_RCU      RCU_GPIOA
#define RS485_DIR_PIN           GPIO_PIN_1
#define RS485_DIR(x) \
    do \
    { \
        if (x) \
            GPIO_BOP(RS485_DIR_PORT) = RS485_DIR_PIN; \
        else \
            GPIO_BC(RS485_DIR_PORT) = RS485_DIR_PIN; \
    } \
    while (0)

/** Debug UART — shared with RS485 port, used by my_printf(...) */
#define DBG_UART                USART1
#define PROTO_UART              USART1
#define DBG_UART_RCU            RCU_USART1
#define DBG_UART_AF             GPIO_AF_7
#define DBG_UART_PORT           GPIOA
#define DBG_UART_PORT_RCU       RCU_GPIOA
#define DBG_UART_TX             GPIO_PIN_2
#define DBG_UART_RX             GPIO_PIN_3
#define DBG_RX_DMA              DMA0
#define DBG_RX_DMA_RCU          RCU_DMA0
#define DBG_RX_DMA_CH           DMA_CH5
#define DBG_RX_DMA_SUBPERI      DMA_SUBPERI4
#define DBG_UART_BAUDRATE       19200U
#define DBG_RXBUF_SIZE          1024U
#define DBG_UART_RDATA_ADDR     ((uint32_t)&USART_DATA(DBG_UART))

/*
 * Protocol / OTA UART (shared with RS485 on USART1).
 * Uses DMA + IDLE interrupt for reception.
 *
 * Common choices on GD32F470:
 *  - USART1 (PA2/PA3 — RS485 on this board)
 *  - USART2 (PB10/PB11)
 *  - UART3 / UART4 / USART5 / UART6 / UART7
 *
 * NOTE:
 * 1) Keep UART AF/pins, DMA controller/channel/subperi, and IRQN matched.
 * 2) Current config uses USART1 on PA2/PA3.
 */
#define PROTO_UART_RCU          RCU_USART1
#define PROTO_UART_IRQN         38  /**< USART1_IRQn, must be a literal for #elif */
#define PROTO_UART_PORT         GPIOA
#define PROTO_UART_PORT_RCU     RCU_GPIOA
#define PROTO_UART_TX_PIN       GPIO_PIN_2
#define PROTO_UART_RX_PIN       GPIO_PIN_3
#define PROTO_UART_AF           GPIO_AF_7
#define PROTO_RX_DMA            DMA0
#define PROTO_RX_DMA_RCU        RCU_DMA0
#define PROTO_RX_DMA_CH         DMA_CH5
#define PROTO_RX_DMA_SUBPERI    DMA_SUBPERI4
#define PROTO_UART_BAUDRATE     19200U
#define PROTO_RXBUF_SIZE        1024U
#define PROTO_UART_RDATA_ADDR   ((uint32_t)&USART_DATA(PROTO_UART))

/** Setup USART1 for RS485 + debug output */
void HalUart_Setup(void);

/** Change USART baudrate at runtime */
void HalUart_SetBaud(uint32_t baudrate);

/** Rearm the protocol UART DMA receiver */
void ProtoUart_DmaRearm(void);

/** Return number of bytes received by the protocol UART DMA */
uint32_t ProtoUart_DmaRxLen(void);

/******************************************************************************/
/* ADC — DMA1 CH4, PC0/PC1/PC2 */

#define ADC_DMA                DMA1
#define ADC_DMA_CH             DMA_CH4
#define ADC_DMA_SUBPERI        DMA_SUBPERI0

#define ADC_PORT               GPIOC
#define ADC_PORT_RCU           RCU_GPIOC

#define ADC_CH_POT             GPIO_PIN_0   /**< PC0 potentiometer input */
#define ADC_CH_DACFB           GPIO_PIN_1   /**< PC1 DAC feedback input */
#define ADC_CH_DACFB_CHANNEL   ADC_CHANNEL_11

#define ADC_VREF_SRC_PC2       1
#if ADC_VREF_SRC_PC2
#define ADC_CH_PT100           GPIO_PIN_2   /**< PC2 PT100 / VREF input */
#define ADC_CH_PT100_CHANNEL   ADC_CHANNEL_12
#else
#define ADC_CH_PT100_CHANNEL   ADC_CHANNEL_17
#endif

/** Running-average sample count */
#define ADC_SAMPLE_AVG         (1)

/** Setup ADC with DMA */
void HalAdc_Setup(void);

/******************************************************************************/
/* DAC — PA4, 12-bit right-aligned */

#define DAC_R12DH_ADDR         (0x40007408)  /**< 12-bit right-aligned DAC holding register address */

#define DAC_PORT               GPIOA
#define DAC_PORT_RCU           RCU_GPIOA

#define DAC_OUT_CH0            GPIO_PIN_4    /**< PA4 DAC channel 0 output */

/** Setup DAC channel 0 */
void HalDac_Setup(void);

/******************************************************************************/
/* RTC — LXTAL with backup-domain magic */

#define RTC_CLK_SRC_LXTAL
#define RTC_BKP_MAGIC          0x32F0

/** Initialise RTC with LXTAL; returns 0 on success */
int HalRtc_Setup(void);

/******************************************************************************/
/* System tick and performance counter */

/** Return the system tick counter in milliseconds */
uint32_t Tick_GetMs(void);

/** Blocking delay for @p ms milliseconds */
void Tick_DelayMs(uint32_t ms);

/** Enable the DWT cycle counter for micro-benchmarking */
void PerfCounter_Init(void);

/******************************************************************************/
/* Extern variable declarations */

extern volatile uint8_t  g_DbgRxBuf[DBG_RXBUF_SIZE];
extern volatile uint16_t g_DbgRxLen;

extern volatile uint8_t  g_ProtoRxBuf[PROTO_RXBUF_SIZE];
extern volatile uint16_t g_ProtoRxLen;

/******************************************************************************/

#ifdef __cplusplus
}
#endif
