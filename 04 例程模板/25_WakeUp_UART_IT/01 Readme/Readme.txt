# CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# Program：CIMC_GD32_WakeUp_UART_IT

## 程序简介
- 工程名称：GD32F470 CIMC_GD32_WakeUp_UART_IT
- 实验平台: CIMC GD32F470 Development Kit V2.0
- MDK版本：5.25

## 板载资源

 - GD32F470VET6 MCU
 
 
## 功能简介
程序模板，可以用来拷贝建立工程
利用GD32F470VET6单片机实现实现 任意中断唤醒 MCU 睡眠模式（Sleep Mode） 的功能。本示例中使用 串口接收中断（USART RX 中断） 作为唤醒源。

## 实验操作
下载程序并复位开发板后，打开串口调试工具，并配置与程序一致的串口参数。系统运行后 MCU 将进入睡眠模式，此时 CPU 停止运行但外设仍保持工作状态。当通过串口调
试工具向开发板发送任意数据时，串口接收中断被触发，从而将 MCU 从睡眠模式中唤醒并继续执行后续程序

## 实验现象
开发板上电或复位后，串口首先打印提示信息，表明 MCU 即将进入睡眠模式。MCU 进入睡眠模式后，当通过串口调试工具发送任意数据时，串口接收中断触发，MCU 被唤醒，
串口打印唤醒提示信息，随后 LED1 被点亮，从而验证通过 串口接收中断唤醒 MCU 睡眠模式 的功能正常。

## 引脚分配

LED1 <---> PA4

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
│  ├─USART
│  └─LED
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
