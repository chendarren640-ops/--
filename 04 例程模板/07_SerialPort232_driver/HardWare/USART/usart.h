/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：usart.h
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/30     V0.01    original
************************************************************/
#ifndef __USART_H__
#define __USART_H__
/************************* 头文件 *************************/

#include "HeaderFiles.h"


/************************* 宏定义 *************************/
#define USART_PORT GPIOD
#define USART USART1
#define USART_TX_Pin GPIO_PIN_5
#define USART_RX_Pin GPIO_PIN_6
#define USART_RCU RCU_USART1
#define USART_PIN_RCU RCU_GPIOD

/************************ 变量定义 ************************/


/************************ 函数定义 ************************/

void usart_init(void);

void usart_recv_buf(void);

#endif // !__USART_H__

/****************************End*****************************/
