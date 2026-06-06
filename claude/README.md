# 2026 CIMC 工业嵌入式系统开发 — 项目工程

> 平台: GD32F470VET6 / CIMC IHD V0.4 | Keil MDK 5.25+ | ST-LINK V2

## 当前状态 ✅

| 模块 | 状态 |
|------|------|
| OLED (128x32 I2C) | ✅ 已验证 — 双行显示正常 |
| LED (PA4 心跳) | ✅ 已验证 — 500ms 闪烁 |
| 编译 | ✅ 0 Error 0 Warning |
| USART0 (CH340 调试) | ✅ printf 可用 |
| USART1 (RS-485) | ⏳ 驱动就绪，待联调上位机 |
| ADC/DAC/RTC/Flash | ⏳ 驱动就绪，待测试 |
| 帧解析/构建/CRC | ⏳ 代码完整，待联调 |
| Bootloader | ⏳ 待编译联调 |

## 快速开始

```
1. 打开 Keil → APP/project/2026CLAUDE_APP.uvprojx
2. 散列文件: APP_debug.sct (0x08000000, 初期调试用)
3. F7 编译 → F8 烧录 (ST-LINK)
4. OLED 显示: 2026263626 / IDLE
5. 串口助手: COM7, 19200-8-N-1, 校验=None
```

## 工程结构

```
claude/
├── APP/                                 Keil 工程: 2026CLAUDE_APP
│   ├── User/          main, systick, gd32f4xx_it, libopt
│   ├── Driver/
│   │   ├── LED/       PA4 心跳, PA5 采集指示
│   │   ├── OLED/      SSD1306 I2C 128x32, OLEDfont.h 完整字库
│   │   ├── USART/     USART0(调试CH340) + USART1(协议RS-485)
│   │   ├── ADC/       CH0(PC0) + CH1(PC1), 连续转换
│   │   ├── DAC/       PA4 DAC0_OUT0, 0~3.3V
│   │   ├── RTC/       LXTAL 32.768kHz, 日历+闹钟
│   │   └── Flash/     参数区 0x08010000, 4KB, CRC32校验
│   ├── Protocol/      ascii_proto, crc16, frame_parser, frame_builder
│   ├── Function/      sys_init, cmd_handler, data_channel, param_mgr, alarm_mgr
│   ├── CMSIS/         ARM Cortex-M4 核心
│   ├── Library/       GD32F4xx 标准外设库 (30个外设)
│   ├── Startup/       startup_gd32f450_470.s
│   ├── project/       .uvprojx + APP.sct + APP_debug.sct
│   └── 开发计划.md      Phase 1~6 详细计划
│
├── Bootloader/                           Keil 工程: 2026CLAUDE_Bootloader
│   ├── main.c         上电→OLED"Bootloader"→延时5s→跳转 0x08011000
│   ├── Driver/        systick_bl, led_bl, oled_bl, usart1_bl
│   ├── project/       Bootloader.sct (0x08000000, 64KB)
│   └── ...
│
├── 01 硬件说明/        PCB 概念图 + 外设图
├── 04 例程模板/        9 个官方训练例程 (LED/KEY/EXTI/串口/ADC/RTC/Flash/FatFS/OLED)
└── README.md
```

## GPIO 引脚分配

| 功能 | GPIO | 模式 | 说明 |
|------|------|------|------|
| LED1 (心跳) | PA4 | 推挽输出 | 低电平点亮, 1s 周期闪烁 |
| LED2 (采集) | PA5 | 推挽输出 | 采集时常亮 |
| OLED SCL | PB8 | 开漏输出 | I2C 模拟 |
| OLED SDA | PB9 | 开漏输出 | I2C 模拟 |
| USART0 TX | PA9 | AF7 | CH340 调试 |
| USART0 RX | PA10 | AF7 | CH340 调试 |
| USART1 TX | PA2 | AF7 | RS-485 协议 |
| USART1 RX | PA3 | AF7 | RS-485 协议 |
| ADC0 CH10 | PC0 | 模拟输入 | 电位器采样 |
| ADC0 CH11 | PC1 | 模拟输入 | DAC 回读 |
| DAC0 OUT | PA4 | 模拟输出 | 需跳接至 PC1 |
| KEY1 | PE2 | 输入上拉 | EXTI2 中断 |

## Flash 地址映射

| 区域 | 起始地址 | 大小 | 用途 |
|------|---------|------|------|
| Bootloader | 0x08000000 | 64KB | 启动 + OTA |
| 参数区 | 0x08010000 | 4KB | 变比/阈值/ID/波特率/告警 |
| APP | 0x08011000 | 128KB | 应用主程序 |
| APP 备份 | 0x08031000 | 128KB | 回滚备份 |
| 固件暂存 | 0x08051000 | 128KB | OTA 接收 |

## 通信协议速查

- **物理层**: RS-485 (USART1, PA2/PA3), 默认 19200-8-N-1
- **编码**: 所有帧字节 → ASCII 十六进制字符串 (0xA5B6 → "A5B6")
- **帧结构**: `A5B6 | DDDD | TT | CCCC | LL | 02 | 内容 | XXXX | B6A5`
- **心跳帧**: `05 8888`, 上电立即发送
- **特殊消息**: 告警/Bootloader/睡眠唤醒 → 纯字符串，不组帧

## 开发阶段

| Phase | 模块 | 分值 | 状态 |
|-------|------|------|------|
| 1 | 通信验证 (心跳帧) | — | ⏳ 进行中 |
| 2 | 基础查询 A/B/C | 19分 | ⏳ |
| 3 | 数据通道 D/E/F/G | 22分 | ⏳ |
| 4 | 自动上报 H/I | 18分 | ⏳ |
| 5 | 高级功能 J/K/L/M | 23分 | ⏳ |
| 6 | Bootloader N | 18分 | ⏳ |

详见 [开发计划.md](APP/开发计划.md)

## 参考资料

- 2026 赛题 PDF: `../1 2026年CIMC工业嵌入式系统开发 初赛 赛题(1).pdf`
- 上位机: `../2 2026CIMC工业嵌入式系统开发 初赛 上位机（正式比赛版）.exe.1`
- 固件升级包: `../固件文件(1).zip`
- 2025 赛题答案: `../2025工业嵌入式开发资料（西门子）/交稿答案/`
- 官方例程: `04 例程模板/`
