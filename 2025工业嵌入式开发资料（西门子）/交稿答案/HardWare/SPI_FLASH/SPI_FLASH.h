/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：SPI_Flash.h
************************************************************/


#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

/************************* 头文件包含 *************************/

#include "HeaderFiles.h"  // 包含项目公共头文件

/************************* 宏定义 *************************/

#define SPI_FLASH_PAGE_SIZE        0x100      // Flash页大小 (256字节)
#define  SPI_FLASH_SECTOR_SIZE     4096
#define SPI_FLASH_CS_LOW()         gpio_bit_reset(GPIOB, GPIO_PIN_12)  // 片选信号置低(选中)
#define SPI_FLASH_CS_HIGH()        gpio_bit_set(GPIOB, GPIO_PIN_12)    // 片选信号置高(取消选中)

/************************* 函数声明 *************************/

/* 初始化SPI1 GPIO和相关参数 */
void spi_flash_init(void);

/* 擦除指定的Flash扇区 */
void spi_flash_sector_erase(uint32_t sector_addr);

/* 擦除整个Flash芯片 */
void spi_flash_bulk_erase(void);

/* 向Flash写入一页数据(最多256字节) */
void spi_flash_page_write(uint8_t* pbuffer, uint32_t write_addr, uint16_t num_byte_to_write);

/* 向Flash写入一块数据(可跨页) */
void spi_flash_buffer_write(uint8_t *pbuffer, uint32_t write_addr, uint32_t num_byte_to_write);

/* 从Flash读取一块数据 */
void spi_flash_buffer_read(uint8_t* pbuffer, uint32_t read_addr, uint16_t num_byte_to_read);

/* 读取Flash芯片的ID */
uint32_t spi_flash_read_id(void);

/* 启动连续读取序列 */
void spi_flash_start_read_sequence(uint32_t read_addr);

/* 从SPI Flash读取一个字节 */
uint8_t spi_flash_read_byte(void);

/* 通过SPI接口发送一个字节并返回接收到的字节 */
uint8_t spi_flash_send_byte(uint8_t byte);

/* 通过SPI接口发送半字(16位)并返回接收到的半字 */
uint16_t spi_flash_send_halfword(uint16_t half_word);

/* 使能Flash写操作 */
void spi_flash_write_enable(void);

/* 轮询等待Flash写操作完成 */
void spi_flash_wait_for_write_end(void);

#endif

