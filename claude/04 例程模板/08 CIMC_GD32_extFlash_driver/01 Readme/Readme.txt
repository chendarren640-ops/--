# 2025年CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# Program：CIMC_GD32_extFlash_driver

## 程序简介
- 工程名称：GD32F470 SPI Flash
- 实验平台: CIMC IHD V0.4
- MDK版本：5.25

## 板载资源

 - GD32F470VET6 MCU
 - GD25Q40ESIGR Flash
 
 
## 功能简介

程序模板，可以用来拷贝建立工程


## 实验操作

1、对Flash进行擦除
2、写入数据，读取后校对
3、写入字符串数据，读取

## 引脚分配

USART0
PA9 - Txd
PA10 - Rxd

Flash
PB12 - Flash cs
PB13 - SCK
PB14 - MISO
PB15 - MOSI

## 程序版本

- 程序版本：V0.1
- 发布日期：2025-04-30

## 联系我们

- Copyright   : CIMC中国智能制造挑战赛
- Author      ：Lingyu Meng
- Website     ：www.siemenscup-cimc.org.cn
- Phone       ：15801122380

## 声明

**严禁商业用途，仅供学习使用。 **


## 目录结构

├─01 Readme		工程项目说明
├─CMSIS			内核驱动文件：Cortex Microcontroller Software Interface Standard
├─Function		用户程序
├─HardWare		硬件驱动
├─HeaderFiles	头文件集合
├─Library		库文件
│  ├─GD32F4xx_standard_peripheral
│  ├─GD32F4xx_usb_library
│  └─Third_Party
├─project		工程文件（含生成的连接文件）
├─Protocol		协议程序
├─Startup		启动文件
├─System		
└─User
