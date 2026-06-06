# 2025年CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# Program：CIMC_GD32_SerialPort_driver

## 程序简介
- 工程名称：CIMC_GD32_SerialPort_driver
- 实验平台: CIMC IHD V0.4
- MDK版本：5.25


## 板载资源

 - GD32F470VET6 MCU
 
 
## 功能简介

实现串口数据收发功能
串口配置详见初始化函数

接收数据做判断：
	·数据为 ‘a’的时候点亮LED2
	·数据为'b'的时候关断LED2
	·其他数据则直接串口打印出来

## 实验操作

上电后，串口输出 sys init
LED1 闪烁

串口接收到数据后，接收数据做判断：
	·数据为 ‘a’的时候点亮LED2
	·数据为'b'的时候关断LED2
	·其他数据则直接串口打印出来


## 引脚分配

PA9  - Txd
PA10 - Rxd
PA4  - LED1
PA5  - LED2


## 程序版本

- 程序版本：V0.1
- 发布日期：2025-03-22

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
