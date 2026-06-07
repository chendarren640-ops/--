/**
 * 2026 CIMC Bootloader — 头文件
 *
 * Flash 布局 (赛题 2.5 节):
 *   Bootloader  0x08000000  64KB
 *   参数区       0x08010000   4KB
 *   APP         0x08011000 128KB
 *   APP 备份    0x08031000 128KB
 *   固件暂存    0x08051000 128KB
 */

#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "gd32f4xx.h"
#include <stdint.h>
#include <string.h>

/* ========== Flash 地址 ========== */
#define APP_START_ADDR       0x08011000
#define PARAM_ADDR           0x08010000
#define STAGING_ADDR         0x08051000
#define FLASH_PAGE_SIZE      4096    /* GD32F4 扇区大小 */

/* ========== OTA 固件魔数 ========== */
#define FW_MAGIC1            0x5A
#define FW_MAGIC2            0xA5
#define FW_MAGIC3            0xC3
#define FW_MAGIC4            0x3C

/* ========== 升级标记 ========== */
#define UPGRADE_FLAG_NONE    0x00
#define UPGRADE_FLAG_REQ     0xA5

/* ========== 超时定义 (ms) ========== */
#define OTA_CMD_TIMEOUT      10000   /* 等待 0x0502 命令超时 (升级模式 10s) */
#define OTA_DATA_TIMEOUT      3000   /* 数据接收完成超时 */
#define NORMAL_BOOT_DELAY     5000   /* 正常启动倒计时 5 秒 (赛题 2.7) */

/* ========== 固件切片大小 ========== */
#define FW_SLICE_SIZE         256

/* ========== LED 引脚 ========== */
#define LED_SYS_PORT     GPIOD
#define LED_SYS_PIN      GPIO_PIN_10  /* PD10: 系统灯, 低电平有效 */

/* ========== OLED 引脚 ========== */
#define OLED_SCL_PORT    GPIOB
#define OLED_SCL_PIN     GPIO_PIN_8
#define OLED_SDA_PORT    GPIOB
#define OLED_SDA_PIN     GPIO_PIN_9

/* ========== 参数区结构体 (与 APP flash_param.h 保持一致) ========== */
typedef struct {
    uint16_t device_id;       /* offset 0 */
    uint8_t  baud_code;       /* offset 2: 13=19200, 14=115200 */
    uint8_t  alarm_mode;      /* offset 3 */
    float    ch0_ratio;       /* offset 4 */
    float    ch1_ratio;       /* offset 8 */
    float    ch0_threshold;   /* offset 12 */
    float    ch1_threshold;   /* offset 16 */
    float    ch2_threshold;   /* offset 20: PT100 温度阈值 */
    uint8_t  upgrade_flag;    /* offset 24: 0xA5=请求升级 */
    uint8_t  reserved[3];     /* offset 25-27 */
    uint32_t crc32;           /* offset 28 */
} ParamBlock_t;

/* ========== 函数声明 ========== */
/* main */
void JumpToApp(uint32_t app_addr);

/* SysTick */
void systick_config(void);
void delay_1ms(uint32_t count);

/* LED */
void LED_Init(void);

/* OLED */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_ShowLine1(uint8_t *str);
void OLED_ShowLine2(uint8_t *str);

/* USART1 */
void USART1_BL_Init(uint32_t baud);
void USART1_SendByte(uint8_t ch);
void USART1_SendString(char *str);
uint8_t USART1_ReadByte(void);
uint16_t USART1_Available(void);
void USART1_Flush(void);

/* Protocol */
void     BL_SendHexFrame(uint8_t *bytes, uint16_t len);
void     BL_SendOK(uint16_t cmd, uint16_t dev_id);
void     BL_SendError(uint16_t dev_id);
uint16_t BL_CRC16(uint8_t *data, uint16_t len);
uint8_t  BL_HexCharToNibble(char c);
uint8_t  BL_ParseHexFrame(uint8_t *out_buf, uint16_t buf_size);

/* Flash */
void BL_FlashWrite(uint32_t addr, uint8_t *data, uint32_t len);
void BL_FlashErase(uint32_t addr, uint32_t page_count);

/* 全局变量 */
extern volatile uint32_t bl_tick;

/* 中断处理 */
void SysTick_Handler(void);
void USART1_IRQHandler(void);

#endif
