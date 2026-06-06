/**
 * @file    usart_driver.h
 * @brief   USART1 串口驱动头文件 (RS485)
 *
 * 赛题要求:
 *   - 通信接口: USART1, RS485
 *   - 默认波特率: 19200-8N1
 *   - 帧格式: 十六进制 → ASCII 字符串收发
 */

#ifndef __USART_DRIVER_H
#define __USART_DRIVER_H

#include "HeaderFiles.h"

/* 默认波特率 - 19200 */
#define USART_BAUDRATE      19200U

/* RS485 方向控制引脚 (根据实际硬件调整) */
/* GD32F470 CIMC-IHD 板上 MAX3485 的 DE/RE 引脚推测为某GPIO */
/* 如果你不确定, 先注释掉, 后续根据硬件调整 */
// #define RS485_DE_PORT    GPIOx
// #define RS485_DE_PIN     GPIO_PIN_x

/* 接收缓冲区大小 */
#define USART_RX_BUF_SIZE   512

/* 接收缓冲区 */
extern volatile uint8_t  usart_rx_buf[USART_RX_BUF_SIZE];
extern volatile uint16_t usart_rx_len;
extern volatile uint8_t  usart_rx_complete;

/* 函数声明 */
void USART1_Config(void);
void USART1_SendString(char *str);
void USART1_SendHexFrame(uint8_t *data, uint16_t len);
int  fputc(int ch, FILE *f);
void process_data(uint8_t data);

#endif
