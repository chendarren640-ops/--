# CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# Program：CIMC_GD32_TimerPWM_driver

## 程序简介
- 工程名称：GD32F470 DEMO 定时器PWM驱动
- 实验平台: CIMC GD32F470 Development Kit V2.0
- MDK版本：5.38


## 板载资源

 - GD32F470VET6 MCU
 
 
## 功能简介

程序模板，可用于拷贝并快速建立工程。
基于 GD32F470VET6 单片机实现：通过 通用定时器 TIMER1_CH1 通道输出 PWM 信号，从而实现 LED 的呼吸灯效果。

## 实验操作

下载程序并复位开发板后，将 CH1 的输出引脚连接到 LED。
连接完成后，即可观察到 LED 亮度周期性变化，呈现呼吸灯效果。

## 引脚分配

PB3 ---> PWM 方波输出引脚

## 程序版本

- 程序版本：V0.1
- 发布日期：2025-12-29

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
│  ├─LED        LED驱动
│  └─Time       定时器驱动
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
