# 2025年CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# Program：CIMC_GD32_DAC_driver

## 程序简介
- 工程名称：GD32F470 DAC驱动
- 实验平台: CIMC GD32F470 Development Kit V2.0
- MDK版本：5.38


## 板载资源

 - GD32F470VET6 MCU
 - 滑动变阻器

## 功能简介

程序模板，可用于拷贝并快速建立工程。
基于 GD32F470VET6 单片机实现：将 ADC 采集到的数字量传递给 DAC，随后由 DAC 将数字量转换为模拟量并输出到 DAC 引脚。

## 实验操作

下载程序并复位开发板后，打开串口调试工具。
系统启动后，三颗 LED 以 1 秒间隔依次点亮后熄灭，随后串口打印 Hello World!。
之后可以调节滑动变阻器，使 ADC 采集的数值发生变化，并传递给 DAC 输出。
可使用万用表测量 DAC 引脚的电压，观察电压值随滑动变阻器调节而产生的变化。


## 引脚分配

PC0 <---> ADC

PA4 <---> DAC

PA5 <---> LED2

PA6 <---> LED3

PA7 <---> LED4


## 程序版本

- 程序版本：V0.1
- 发布日期：2025-12-31

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
│  ├─ADC		ADC驱动
│  ├─DAC		DAC驱动
│  ├─LED		LED驱动
│  └─USART0		串口驱动
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
