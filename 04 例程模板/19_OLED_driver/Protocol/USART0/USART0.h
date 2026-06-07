/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：usart0.h
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/3/10     V0.01    original
************************************************************/


#ifndef __USART0_H
#define __USART0_H

/************************* 头文件 *************************/

#include "HeaderFiles.h"

/************************ 变量定义 ************************/

//bool data_flag = false;

/************************ 函数定义 ************************/

void USART0_Config(void);  							// 串口功能配置
void USART0_SendData(uint16_t *buf,uint16_t len);   // 发送数据
void process_data(uint8_t data);					// 数据处理
		 				    
#endif


/****************************End*****************************/

