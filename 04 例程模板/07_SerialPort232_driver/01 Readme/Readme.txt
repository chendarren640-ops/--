# CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# Program：SerialPort232_driver

## 程序简介
- 工程名称：GD32F470 DEMO 串口接收中断驱动
- 实验平台: CIMC GD32F470 Development Kit V2.0
- MDK版本：5.38


## 板载资源

 - GD32F470VET6 MCU
 
 
## 功能简介

程序模板，可用于拷贝并快速建立工程。
基于 GD32F470VET6 单片机实现：通过232 进行串口1通信 

## 实验操作

下载程序并复位开发板后，打开串口调试工具（需要使用232调试）。
USART1_TX 需要连接 232_RX
USART1_RX 需要连接 232_TX
GND 需要连接 GND
上述完成之后，通过串口调试工具跑232，向MCU发送数据，此时串口调试工具会打印 232_RX:ok\r\n  recv data : (发送的信息) len : (发送信息的长度)
以及周期性打印 232_TX : ok\r\n

此时printf 重载的是 usart1发送功能 

## 引脚分配


PD5 <---> USART1_TX
PD6 <---> USART1_RX

## 程序版本

- 程序版本：V0.1
- 发布日期：2025-12-30

## 联系我们

- Copyright   : CIMC中国智能制造挑战赛
- Author      ：Lingyu Meng
- Website     ：www.siemenscup-cimc.org.cn
- Phone       ：15801122380

## 声明

**严禁商业用途，仅供学习使用。 **


## 目录结构

├─01 Readme         工程项目说明
├─CMSIS             内核驱动文件：Cortex Microcontroller Software Interface Standard
├─Function          用户程序
├─HardWare          硬件驱动
│  ├─LED            LED驱动
│  └─USART          USART驱动
├─HeaderFiles       头文件集合
├─Library           库文件
│  ├─GD32F4xx_standard_peripheral
│  ├─GD32F4xx_usb_library
│  └─Third_Party
├─project           工程文件（含生成的连接文件）
├─Protocol          协议程序
├─Startup           启动文件
├─System
└─User
