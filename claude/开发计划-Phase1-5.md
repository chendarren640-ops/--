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

## Phase 4: 稳定性 + 赛场准备

1. 连续 3 轮评测 38/38 通过
2. OLED 显示队伍号
3. 删除调试 printf, 0 Error 0 Warning
4. `git tag v3.0-final` 打标签
5. 备份 hex 文件
6. GitHub 仓库整理（分支清理 + 推送 tags）

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
