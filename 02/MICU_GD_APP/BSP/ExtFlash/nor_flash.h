/* SPDX-License-Identifier: MIT */

/**
 * @file    nor_flash.h
 * @brief   GD25Qxx serial NOR flash driver — SPI DMA interface.
 *
 * Provides sector erase, bulk erase, buffer read/write, and ID read
 * operations for the GD25Qxx family.
 */

#pragma once

#include "gd32f4xx.h"
#include "gd32f4xx_spi.h"
#include "gd32f4xx_gpio.h"
#include "board_defs.h"

#define NOR_FLASH_PAGE_SZ  0x100

/** Chip select control — PB12 */
#define NOR_FLASH_CS_LOW()   gpio_bit_reset(GPIOB, GPIO_PIN_12)
#define NOR_FLASH_CS_HIGH()  gpio_bit_set(GPIOB, GPIO_PIN_12)

/** SPI peripheral — SPI1 */
#define NOR_FLASH_SPI         SPI1

#define FLASH_DMA_TXBUFSZ     (12)

/* GD25Qxx command set */
#define FLASH_CMD_WRITE       0x02  /**< Write to memory */
#define FLASH_CMD_WRSR        0x01  /**< Write status register */
#define FLASH_CMD_WREN        0x06  /**< Write enable */
#define FLASH_CMD_READ        0x03  /**< Read from memory */
#define FLASH_CMD_RDSR        0x05  /**< Read status register */
#define FLASH_CMD_RDID        0x9F  /**< Read identification */
#define FLASH_CMD_ERASE_4K    0x20  /**< Sector erase (4 KiB) */
#define FLASH_CMD_ERASE_CHIP  0xC7  /**< Bulk / chip erase */

#define FLASH_WIP_FLAG        0x01  /**< Write-in-progress bit in SR */
#define FLASH_DUMMY_BYTE      0xA5

void NorFlash_Test(void);

/** Initialize the SPI Flash (de-assert CS and enable SPI) */
void NorFlash_Init(void);

/** Erase a 4 KiB sector at @p sector_addr */
void NorFlash_EraseSector(uint32_t sector_addr);

/** Erase the entire flash chip */
void NorFlash_EraseChip(void);

/** Write a block to flash (handles page crossing) */
void NorFlash_Write(uint8_t *pbuffer, uint32_t write_addr, uint16_t num_byte_to_write);

/** Read a block from flash */
void NorFlash_Read(uint8_t *pbuffer, uint32_t read_addr, uint16_t num_byte_to_read);

/** Read the JEDEC manufacturer/device ID */
uint32_t NorFlash_ReadId(void);

/** Begin a read sequence (CS low + READ cmd + address) */
void NorFlash_StartReadSeq(uint32_t read_addr);

/** Send write-enable command */
void NorFlash_WriteEnable(void);

/** Poll WIP flag until the device is idle */
void NorFlash_WaitBusy(void);

/* DMA transfer helpers */
uint8_t  NorFlash_SendByteDma(uint8_t byte);
uint16_t NorFlash_SendHalfDma(uint16_t half_word);
void     NorFlash_XferDma(uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t size);
void     NorFlash_WaitDmaEnd(void);
