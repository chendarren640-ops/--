# CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# Program：CIMC_GD32_IndependentWatchdog_driver

## 程序简介
- 工程名称：GD32F470 DEMO 看门狗驱动模板
- 实验平台: CIMC GD32F470 Development Kit V2.0
- MDK版本：5.38


## 板载资源

 - GD32F470VET6 MCU
 
 
## 功能简介
程序模板，可以用来拷贝建立工程
利用GD32F470VET6单片机实现 通过基本定时器和按键控制进行周期性喂狗（看门狗刷新） 的功能。下载程序并复位开发板后，打开串口调试工具，并将串口参数配置为与程
序一致。

## 实验操作
下载程序后复位开发板，打开串口调试工具

系统运行后将通过基本定时器周期性执行喂狗操作，同时串口会打印相关状态信息。当通过按键停止喂狗后，如果在规定时间内未再次喂狗，看门狗将
触发超时事件，触发 看门狗中断并执行软复位，系统重新启动，从而验证看门狗监控与复位功能正常。

## 引脚分配

PE4 ---> Key

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
│  ├─FWDGT      看门狗驱动
│  ├─KEY        按键驱动
│  ├─LED        LED驱动
│  ├─Time       定时器驱动
│  └─USART0     串口驱动
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
