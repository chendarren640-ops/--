# CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# CIMC_GD32_SerialPortDMA_driver

## 程序简介
- 工程名称：GD32F470 DEMO 串口收发DMA驱动
- 实验平台: CIMC GD32F470 Development Kit V2.0
- MDK版本：5.38


## 板载资源

 - GD32F470VET6 MCU
 
 
## 功能简介

程序模板，可用于拷贝并快速建立工程。
基于 GD32F470VET6 单片机实现：通过 串口结合 DMA 方式进行数据接收与发送，并根据接收到的数据执行相应的控制功能。


## 实验操作
下载程序并复位开发板后，打开串口调试工具。
随后通过串口调试工具发送 OK / ok（点亮 LED1）或 OFF / off（熄灭 LED1）。
同时串口会打印接收到的数据信息以及对应的数据长度。

## 引脚分配

LED1 <---> PA4

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
├─HardWare
│  ├─DMA            DMA驱动
│  ├─LED            LED驱动
│  └─USART          串口驱动
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
