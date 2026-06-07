# CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# Program：CIMC_GD32_OLED

## 程序简介
- 工程名称：GD32F470 DEMO 程序模板
- 实验平台: CIMC IHD V0.4
- MDK版本：5.25

## 板载资源

 - GD32F470VET6 MCU
 - OLED
 
 
## 功能简介

程序模板，可以用来拷贝建立工程
基于 GD32F470VET6 单片机实现：通过 I2C 实现 OLED显示指定信息，包括ADC的数据采集和指定字符串的打印，
其中ADC是上述实验项目中的逻辑实现。

注意：I2C 是通过软件模拟实现的，而不是硬件实现，所以就没有开启复用模式。

## 实验操作

下载程序并复位开发板后，观测OLED上显示的内容。


## 引脚分配

OLED_CLK --- PB8
OLED_DAT --- PB9

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
├─0 取模软件PCtoLCD2002
├─CMSIS			内核驱动文件：Cortex Microcontroller Software Interface Standard
├─Delay
├─HardWare		硬件驱动
│  ├─KEY
│  ├─LED
│  └─OLED
├─HeaderFiles	头文件集合
├─Implement		用户程序
├─Library		库文件
├─project		工程文件（含生成的连接文件）
├─Protocol		协议程序
│  └─USART0
├─Readme		工程项目说明
├─Startup		启动文件
├─System
│  └─ADC
└─User

