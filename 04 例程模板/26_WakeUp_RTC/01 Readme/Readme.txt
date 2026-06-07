# CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# Program：WakeUp_RTC

## 程序简介
- 工程名称：GD32F470 WakeUp_RTC
- 实验平台: CIMC GD32F470 Development Kit V2.0
- MDK版本：5.25


## 板载资源

 - GD32F470VET6 MCU
 
 
## 功能简介

程序模板，可以用来拷贝建立工程
利用GD32F470VET6单片机实现RTC 闹铃唤醒功能：MCU 进入 待机模式（Standby Mode） 后，通过 RTC 闹铃事件 将 MCU 从待机模式中唤醒。

## 实验操作
下载程序并复位开发板后，系统开始运行。此时 LED1 点亮约 3 秒后熄灭，随后 MCU 进入 待机模式。约 3 秒后 RTC 闹铃触发，MCU 被唤醒并产生复位，程序重新开始
执行。系统将按照上述流程 循环运行，从而验证 RTC 闹铃唤醒 MCU（待机模式） 的功能是否正常。

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
│  ├─LED		LED驱动
│  ├─RTC		RTC驱动
│  └─USART		串口驱动
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
