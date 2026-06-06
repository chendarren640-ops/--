
#ifndef __FUNCTION_H
#define __FUNCTION_H

/************************* 头文件 *************************/

#include "HeaderFiles.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/


/************************ 函数定义 ************************/

void System_Init(void);      	// 系统初始化
void UsrFunction(void);         // 用户函数
void Init_LED_Stat(void);		// 系统初始化时用LED显示状态

#define BUFFER_SIZE              256
#define TX_BUFFER_SIZE           (countof(tx_buffer) - 1)
#define RX_BUFFER_SIZE           0xFF

#define countof(a)               (sizeof(a) / sizeof(*(a)))

#define SFLASH_ID                0xC84013
#define TEAM_ID                  2025239771
#define FLASH_WRITE_ADDRESS      0x000000
#define FLASH_READ_ADDRESS       FLASH_WRITE_ADDRESS



void turn_on_led(uint8_t led_num);
void get_chip_serial_num(void);
ErrStatus memory_compare(uint8_t *src, uint8_t *dst, uint16_t length);
void test_status_led_init(void);
void flash_init(void); 


#endif


/****************************End*****************************/

