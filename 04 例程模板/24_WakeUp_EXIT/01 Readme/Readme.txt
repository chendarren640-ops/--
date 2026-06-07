# CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# Program：CIMC_GD32_WakeUp_EXIT

## 程序简介
- 工程名称：CIMC_GD32_WakeUp_EXIT
- 实验平台: CIMC GD32F470 Development Kit V2.0
- MDK版本：5.25


## 板载资源

 - GD32F470VET6 MCU
 
 
## 功能简介
程序模板，可以用来拷贝建立工程
利用GD32F470VET6单片机实现 外部中断唤醒 MCU 深度睡眠模式（Deep-Sleep Mode） 的功能


## 实验操作
下载程序并复位开发板后，系统开始运行，约 2 秒后 MCU 进入深度睡眠模式。在深度睡眠模式下，CPU 及大部分系统时钟被关闭，以降低功耗。此时可通过按下开发板上的 
KEY1 按键 触发外部中断，从而将 MCU 从深度睡眠模式中唤醒，并继续执行后续程序。

## 实验现象

开发板上电后，LED1 首先点亮，约 2 秒后 LED1 熄灭，表示 MCU 已进入深度睡眠模式。当按下 KEY1 按键 触发外部中断后，MCU 被唤醒，此时 LED1 开始以 1 秒
周期进行闪烁，从而验证 通过外部中断唤醒 MCU 深度睡眠模式 的功能正常。


## 引脚分配

KEY1 -- PE3

LED1 -- PA5

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
│  ├─KEY
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
