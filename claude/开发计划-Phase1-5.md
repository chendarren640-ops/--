# 2026 CIMC 开发计划 — 全 38 项通关

## 当前 Git 状态

```
dac_fix (HEAD)      a19418c  ★ 4.21: DAC 复刻西风, A~K 通过
rs485_fix            ae18018  旧 (已合并到 dac_fix)
main                 39b65c9  二进制 parser (有问题)
old_version          9660598  初始模板
```

**当前分支**: `dac_fix`  
**DE 引脚**: PA1（硬件原理图确认）  
**通信口**: USART1 (PA2/PA3), RS-485, 19200-8-N-1

---

## Phase 2: 修复 L/M 模块

> **当前**: A~K 已通过（dac_fix commit a19418c）  
> **目标**: 补完 L + M，冲到 N 之前（≈82 分）

### 2.1 L 模块 — 修改设备 ID (6分)
- 命令: `0x01A1` → 2 字节新 ID → 写 Flash → 回复新 ID
- 重启后 `Param_Load` 自动恢复新 ID
- `Build_IDReply` 用 `cmd` 变量，不硬编码

### 2.2 M 模块 — 修改波特率 (6分)
- 命令: `0x01A2` → 1 字节波特率码(13=19200, 14=115200) → 写 Flash → OK → 切换
- 上位机同步切波特率，继续通信
- 重启后 `Param_Load` → `USART1_Config_Baud` 恢复新波特率

**验证**: A~M 全部通过（≈82 分）

---

## Phase 3: Bootloader OTA (N 模块 18 分)

> **目标**: APP+Bootloader 完整 OTA 流程，冲 100 分

### 3.1 Bootloader 工程对齐
- 通信口: USART1 (PA2/PA3), 19200, DE=PA1
- OLED 显示倒计时
- 升级标记检测: Flash 参数区 `upgrade_flag == 0xA5`

### 3.2 OTA 流程
```
上位机发 0x0501 → APP 设 upgrade_flag=0xA5 + Param_Save + OK + NVIC_SystemReset
    ↓
Bootloader 启动 → 检测 upgrade_flag=0xA5:
    ├─ OLED 显示倒计时
    ├─ 等上位机发 0x0502
    ├─ 收 256 字节切片 bin 文件
    ├─ 校验魔术字 5A A5 C3 3C
    │   ├─ 正确 → 搬运到 APP 区(0x08011000) → 清 upgrade_flag → OK → 跳转
    │   └─ 错误 → 回复 FF EEEE → 跳转原 APP
    └─ 超时 10s → 跳转原 APP
```

### 3.3 升级后验证
- 刷回 APP 不响应 0x0501
- 正常响应其他命令

**验证**: 全 38 项通过（100 分）

---

## Phase 4: 人检项 + 稳定性 + 赛场准备

### 4.1 OLED 双行显示（裁判目视/摄像头）

> **赛题要求**: 第 1 行显示队伍编号, 且需要根据系统状态**动态变化**

| 状态 | Line1 (队伍号) | Line2 (状态) |
|------|---------------|-------------|
| 上电初始化 | `2026CIMC` | `IDLE` |
| 正常待命 | `2026CIMC` | `IDLE` |
| 自动上报中 | `2026CIMC` | `SAMPLING` |
| 告警触发 | `2026CIMC` | `ALARM!` |
| 深度睡眠前 | `2026CIMC` | `SLEEP` |
| 升级模式 | `2026CIMC` | `UPGRADE` |

改动点: `sys_init.c` + `main.c` 主循环中根据 `g_sys_state` + `alarm_count` 刷新 OLED

### 4.2 LED PA4 心跳（裁判目视）

- PA4 1s 周期闪烁（`SysTick` 控制，已实现）
- PA5 采集指示（采集时常亮，可选）

### 4.3 Bootloader OLED 倒计时

Bootloader 启动时 OLED 显示:
```
Line1: "Bootloader"
Line2: "Wait 10s..."
       "Wait  7s..."
       "Wait  4s..."
       "Wait  1s..."
       "Jump APP"
```
收到 0x0502 后:
```
Line1: "Bootloader"
Line2: "Receiving..."
```
搬运中:
```
Line1: "Bootloader"
Line2: "Writing..."
```
完成:
```
Line1: "Bootloader"
Line2: "Done! Jump.."
```

### 4.4 代码文件夹结构审查

赛题要求工程下必须有以下三层目录:
```
APP/
├── Driver/     ← 所有硬件驱动 (.c/.h)
├── Protocol/   ← 帧解析/构建/CRC/ASCII
├── Function/   ← 业务逻辑
├── User/       ← main.c + 中断
├── Library/    ← GD32 标准外设库 (不动)
├── CMSIS/      ← ARM 核心 (不动)
├── Startup/    ← 启动文件 (不动)
└── project/    ← Keil 工程文件
```
审查前确认: 没有把驱动写到 Protocol 里, 没有把协议写到 Function 里。

### 4.5 硬件模块评分

| 硬件项 | 评分方式 | 状态 |
|--------|---------|------|
| GD32F470 最小系统 | 裁判目视 PCB | 自研板需确认丝印清晰 |
| RS-485 (MAX3485 + DE) | 上位机通信验证 | PA1 已确认 |
| CH340 USB-UART | 调试打印可用 | ✅ |
| OLED 128×32 I2C | 裁判目视显示 | ✅ |
| LED ×4 (PA4~PA7) | 裁判目视闪烁 | PA4心跳已实现 |
| ADC 电位器 (PC0) | 上位机 CH0 数据 | ✅ |
| DAC 输出 (PA4) | 上位机 + 跳线 | PA4→PC1 跳线已接 ✅ |
| RTC 时钟 (32.768kHz) | 上位机时间查询 | 已修复 |
| Flash 参数存储 | 重启持久化验证 | ✅ |
| 按键 (PE2~PE5) | 裁判检查 | 可选 |

### 4.6 项目书 + PPT

| 文档 | 内容要点 |
|------|---------|
| **项目书 (Word)** | 系统方案、硬件框图、软件架构(Driver/Protocol/Function 三层)、通信协议、Flash 地址映射、外设引脚分配表、评测结果截图 |
| **PPT** | 团队分工、技术亮点(OTA/自动上报/告警)、硬件展示、38 项评测通过证明、创新点 |
| **原理图/PCB** | 自研板需提交。标注 MCU 引脚复用、电源、RS-485、OLED、CH340 等关键电路 |

> 建议: 38 项跑通后集中半天整理文档。**项目书里放评测日志截图**是最有说服力的。

### 4.7 最终检查清单

1. 连续 3 轮评测 38/38 通过
2. OLED 双行跟随状态变化
3. LED PA4 1s 闪烁
4. Bootloader OLED 倒计时完整
5. 删除调试 printf, 0 Error 0 Warning
6. 代码三层目录规范
7. 项目书 + PPT 定稿
8. `git tag v3.0-final` 打标签
9. 备份 hex + 项目书 + PPT

---

## 关键教训速查

| 教训 | 来源 |
|------|------|
| 收发必须同路 | USB 测试时 SendHexFrame→USART1, 接收→USART0, 上位机等不到回复 |
| 所有命令 type=0x01 | 上位机一律用 TT=0x01 发请求 |
| 应答命令字=请求命令字 | 上位机发 0x0400, 回 0x0421→不认 |
| SysTick=1ms 不要 *2 | systick_config(SystemCoreClock/1000) |
| 命令字从日志反推 | 0x0241/0x0242/0x0261/0x0400 都是猜错的 |
| DE 引脚听硬件的 | PA1, 不是 PE1/PC13/PB12 |
| 改完一个分支要锁 | 不同 AI 窗口互相覆盖是回归最大来源 |
