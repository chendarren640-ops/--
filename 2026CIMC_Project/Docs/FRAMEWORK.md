# 2026CIMC 项目框架需求文档

## 一、整体架构

两个完全独立的 Keil 工程，不共用任何源文件。

```
2026CIMC_Project/
├── APP/                    工程文件: APP/project/2026CIMC_APP.uvprojx
│   ├── User/               程序入口 + 中断 + SysTick
│   ├── Driver/             ★ 赛题要求必须在这里: UART/I2C/SPI/OLED/Flash/ADC 等驱动
│   ├── Protocol/           ★ 赛题要求必须在这里: 帧解析/组帧/CRC16
│   ├── Function/           ★ 赛题要求必须在这里: 采样/变比/阈值/告警/参数管理
│   ├── CMSIS/              ARM 核心 + GD32F470 头文件 (不修改)
│   ├── Library/            GD32F4xx 标准外设库 (不修改)
│   └── Startup/            启动文件 (不修改)
│
├── Bootloader/             工程文件: Bootloader/project/2026CIMC_Bootloader.uvprojx
│   ├── main.c              主入口
│   ├── bootloader.h        公共头文件
│   ├── Driver/             精简驱动: SysTick + LED + OLED + USART1
│   ├── CMSIS/              (同上)
│   ├── Library/            (精简: 仅 GPIO/RCU/MISC/PMU/FMC/USART)
│   └── Startup/            (同上)
│
└── Docs/                   文档
    ├── FRAMEWORK.md        本文件
    └── PROTOCOL.md         协议速查表
```

## 二、Flash 地址映射 (必须严格遵守, 违反扣分)

| 区域         | 起始地址     | 大小   | Keil ROM 配置          |
|-------------|------------|--------|------------------------|
| Bootloader  | 0x08000000 | 64KB   | IROM1: 0x8000000/0x10000 |
| 参数区       | 0x08010000 | 4KB    | (不在工程里, 由代码直接读写) |
| APP         | 0x08011000 | 128KB  | IROM1: 0x8011000/0x20000 |
| APP备份区   | 0x08031000 | 128KB  | (OTA用, 不在工程里)       |
| 固件暂存区  | 0x08051000 | 128KB  | (OTA接收区, 不在工程里)   |

## 三、通信接口 (关键: 用 USART1 不是 USART0)

| 参数       | 值                     |
|-----------|------------------------|
| 外设       | **USART1** (RS-485)   |
| 默认波特率 | **19200**-8-N-1        |
| TX 引脚   | PA2 (AF7)              |
| RX 引脚   | PA3 (AF7)              |
| 编码方式  | 所有帧字节 → ASCII 十六进制字符串 |

## 四、通信协议帧格式

```
[A5B6][DDDD][TT][CCCC][LL][02][内容N字节][XXXX][B6A5]
  2B    2B   1B   2B   1B  1B     NB       2B    2B
起始  设备ID 类型 命令字 长度 版本  数据    CRC16  结束
```

- CRC16-Modbus: 计算范围 = 起始标志 → 内容末尾
- 0xA5B6 发送为 ASCII 字符 `A5B6` (4个字节)
- 部分应答不组帧, 直接发字符串: 告警记录/Bootloader 倒计时/睡眠唤醒

## 五、评分关键模块优先级

| 优先级 | 模块         | 分值 | 依赖                  |
|--------|-------------|-----|-----------------------|
| P0     | 串口19200启动 | -   | 否则全0分              |
| P0     | 心跳帧上电发送 | 1分  | USART1初始化后立即发   |
| P0     | 基础查询B     | 13分 | ADC/DAC/RTC           |
| P1     | 变比+阈值D/E  | 12分 | Flash参数区            |
| P1     | 重启持久化F/G | 10分 | Flash参数区            |
| P1     | 自动上报H     | 11分 | RTC + 定时器           |
| P2     | 告警I         | 7分  | Flash存储              |
| P2     | 睡眠唤醒J     | 5分  | RTC闹钟 + PMU深度睡眠  |
| P2     | 异常处理K     | 6分  | CRC校验                |
| P3     | 修改ID/波特率L/M | 6+6分 | Flash参数区          |
| P3     | Bootloader N  | 18分 | OTA整体流程            |

## 六、APP 各层职责

### Driver/ (底层驱动, 不含业务逻辑)
- `usart1_drv.c/h`   — USART1 19200 RS-485, 环形缓冲中断接收, printf重定向
- `led_drv.c/h`      — PA4心跳1s闪烁, PA5采集指示常亮
- `oled_drv.c/h`     — SSD1306 I2C 128x32, 双行文本
- `adc_drv.c/h`      — CH0(PC0/ADC0_CH10) + CH1(PC1/ADC0_CH11) 连续DMA采集
- `dac_drv.c/h`      — PA4/DAC0_OUT0, 0~4095 → 0~3.3V
- `flash_param.c/h`  — 内部Flash 0x08010000 参数区读写
- `rtc_drv.c/h`      — UTC时间戳设置/读取 + RTC闹钟唤醒

### Protocol/ (帧处理, 不含业务逻辑)
- `ascii_proto.c/h`  — Byte↔HexStr 互转, SendHexFrame
- `crc16.c/h`        — CRC16-Modbus
- `frame_parser.c/h` — 接收缓冲区帧检测、命令分发
- `frame_builder.c/h`— 各命令的应答帧构造函数

### Function/ (业务逻辑, 调用Driver和Protocol)
- `sys_init.c/h`     — System_Init() 统一初始化
- `cmd_handler.c/h`  — 命令处理总入口 (switch命令字)
- `data_channel.c/h` — CH0/CH1变比计算, 阈值判断, 自动上报
- `param_mgr.c/h`    — 参数结构体定义, 读写Flash参数区
- `alarm_mgr.c/h`    — 告警记录存储/查询/清除

## 七、Bootloader 职责

```
上电
 ├─ 初始化: SysTick + LED + OLED + USART1
 ├─ 检查升级标记 (Flash参数区 0x08010000 某字节)
 │   ├─ 无标记: 不输出任何信息, 延时5s → 跳转 0x08011000
 │   └─ 有标记: 打印倒计时提示, 等待10s
 │       ├─ 收到 0x0502 (准备传输): 接收256字节切片 bin 文件
 │       │   ├─ 校验前4字节魔术字 5A A5 C3 3C
 │       │   ├─ 正确: 搬运到 0x08011000, 回复OK, 跳转
 │       │   └─ 错误: 回复错误帧 FF EEEE, 跳转原APP
 │       └─ 超时无命令: 跳转 0x08011000
```

跳转方式:
```c
__disable_irq();
__set_MSP(*(uint32_t*)APP_ADDR);
((void(*)(void))(*(uint32_t*)(APP_ADDR+4)))();
```

## 八、参数区结构 (0x08010000, 4KB)

```c
typedef struct {
    uint16_t device_id;      // 设备ID, 默认自行设置
    uint8_t  baud_code;      // 波特率编码: 13=19200, 14=115200
    uint8_t  alarm_mode;     // 0x01=主动上报, 0x02=仅存储
    float    ch0_ratio;      // CH0变比, 默认1.0
    float    ch1_ratio;      // CH1变比, 默认1.0
    float    ch0_threshold;  // CH0阈值
    float    ch1_threshold;  // CH1阈值
    uint8_t  upgrade_flag;   // OTA标记: 0xA5=需要升级
    uint8_t  reserved[3];
    uint32_t crc;            // 参数区自身CRC32校验
} ParamBlock_t;              // 写入时需先擦除整个扇区
```

## 九、已知问题 / 待修正

1. **USART0 → USART1**: 原来写的 `usart0_drv.c` 错用了 USART0, 需改为 USART1 (PA2/PA3, AF7)
2. **DAC引脚冲突**: CH1 = DAC回读 (PA4→PC1跳接), DAC输出在 PA4, ADC回读在 PC1 (ADC0_CH11)
3. **CH2 PT100**: 外部ADC通道, 命令字 0x0221, 需接测试板
4. **固件版本**: 初始版本号 `2.0.1.0` = 字节 `02 00 01 00`
5. **oled_drv.c 缺字库**: 需从2025答案复制 OLEDfont.h 到 Driver/
