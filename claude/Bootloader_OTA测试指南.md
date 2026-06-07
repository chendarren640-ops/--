# Bootloader + OTA 测试指南

> 基于最新评测日志，逐步骤验证，预期现象写得很细

---

## 前置准备

```
1. 硬件: GD32F470VET6 开发板 + ST-LINK V2 + RS-485 转 USB
2. 软件: Keil MDK 打开两个工程:
   - Bootloader: claude\Bootloader\project\2026CLAUDE_Bootloader.uvprojx
   - APP:       claude\APP\project\2026CLAUDE_APP.uvprojx
```

---

## 阶段 1: Bootloader 编译烧录

### 1.1 编译 Bootloader
```
1. 打开 Bootloader 工程 → F7 编译
   ✅ 预期: 0 Error, 0 Warning
   
   ❌ 如果报 proto_bl.c 找不到:
      右键 Target → Add Existing Files → 添加:
      claude\Bootloader\Driver\proto_bl.c
      claude\Bootloader\Driver\proto_bl.h
```

### 1.2 烧录 Bootloader
```
1. F8 烧录 Bootloader
2. 烧录完成后, OLED 应显示:
   ┌────────────────┐
   │  2026CIMC      │
   │  Bootloader    │
   └────────────────┘
   
3. 5 秒后屏幕不变 (因为没有 APP, "No APP!")
   LED PA4 快闪 (200ms 周期)
```

---

## 阶段 2: APP 编译烧录 (配合 Bootloader)

### 2.1 切换 APP 的散列文件
```
Keil → Project → Options for Target → Linker → 散列文件改为:
  claude\APP\project\APP.sct   (0x08011000 启动)
  
⚠️ 注意: 之前调试用的是 APP_debug.sct (0x08000000),
   现在 Bootloader 占用了 0x08000000, 必须改用 APP.sct
```

### 2.2 检查 APP 代码改动
```
确认以下文件已更新 (本次改动):
 ✅ sys_init.c       — USART1_Config_Baud() 波特率持久化
 ✅ cmd_handler.c    — M模块立即切换波特率 + 0x0401/0x0402 读阈值
```

### 2.3 编译烧录 APP
```
1. F7 编译 → ✅ 0 Error, 0 Warning
2. F8 烧录 (APP 写入 0x08011000, 不会覆盖 Bootloader)
3. 烧录完成后 OLED:
   ┌────────────────┐
   │  2026CIMC      │
   │  Bootloader    │  ← 持续 5 秒
   └────────────────┘
   5 秒后 →
   ┌────────────────┐
   │  2026CIMC      │
   │  IDLE          │  ← APP 启动!
   └────────────────┘
```

---

## 阶段 3: 核心 Bug 修复验证

### 测试 3.1: 验证 printf 不再混入 RS-485 协议帧
```
1. 打开上位机, 串口 COM7, 19200-8-N-1
2. 给设备断电再上电
3. 上位机查看收到的原始数据:
   ✅ 预期: 只看到 A5B60DE0058888... 干净的心跳帧
   ❌ 之前: 会看到 "OLED OK USART1 OK..." 混在帧数据里
```

### 测试 3.2: 验证 M 模块波特率切换 (不再失联!)
```
1. 上位机 → 查询设备 ID (广播): A5B6FFFF0111000201B0B6A5
   ← 应收到设备 ID 回复

2. 上位机 → 修改波特率为 115200:
   → A5B6XXXX01A2010214CRC_B6A5
   ← 应收到 OK (在旧波特率 19200 回复)

3. 上位机切换到 115200:
   上位机关闭串口 → 重新打开 → 波特率选 115200

4. 上位机 → 查询设备 ID:
   ← 应收到设备 ID 回复！
   ✅ 如果收到 → M 模块正常
   ❌ 如果超时 → 波特率切换失败 (检查 usart_drv.c 的 USART1_Config_Baud)
```

### 测试 3.3: 验证阈值单独查询
```
1. 上位机 → 读 CH0 阈值 (0x0401):
   → A5B6XXXX0104010002CRC_B6A5
   ← 应返回 4 字节 IEEE 754 浮点数 + CRC
   ✅ 正确: A5B6XXXX0204010402XXXXXXXXCRC_B6A5 (4 字节数据)
   ❌ 之前: 只返回 FF (OK 没有数据)
```

---

## 阶段 4: Bootloader OTA 功能验证

### 测试 4.1: Normal Boot (不触发 OTA)
```
1. 设备上电 → OLED: "2026CIMC / Bootloader" (5秒)
2. 5 秒后 → OLED: "2026CIMC / IDLE"
3. LED PA4 500ms 心跳闪烁
4. 上位机收到心跳帧
   ✅ 正常启动完成
```

### 测试 4.2: OTA 升级请求
```
1. APP 正常运行 (OLED 显示 IDLE)
2. 上位机 → 发送 OTA 升级请求:
   → A5B6XXXX0105010002CRC_B6A5  (0x0501)
   
3. 设备应:
   ← 回复 OK (0x02 0501 01 FF)
   → 立即重启
   
4. 重启后, OLED 显示:
   ┌────────────────┐
   │  2026CIMC      │
   │  OTA Mode      │  ← Bootloader 检测到 upgrade_flag!
   └────────────────┘
   然后发送心跳帧

5. 上位机收到心跳 (证明 Bootloader RS-485 通信正常)
   ✅ OTA 模式进入成功
```

### 测试 4.3: 等待 0x0502 命令
```
1. Bootloader 显示 "Wait 0502..."
2. 上位机 → 发送 0x0502:
   → A5B6XXXX0105020002CRC_B6A5
   
3. 设备应:
   ← 回复 OK (0x02 0502 01 FF)
   
4. OLED 显示 "Recv FW..."
   ✅ 进入固件接收模式
```

### 测试 4.4: 接收正确固件
```
1. 上位机发送固件数据 (ASCII 十六进制流)
   固件前 4 字节必须是: 5A A5 C3 3C (魔术字)
   
2. 设备接收过程中, OLED 显示 "Recv FW..."

3. 上位机停止发送 3 秒后, 设备自动检测传输结束

4. 收到正确魔术字后:
   OLED: "Write Flash" → "Copy to APP" → "OTA OK!"
   然后跳转到新 APP
   ✅ 升级完成!

5. 新 APP 启动, OLED 显示 "IDLE", 心跳正常
```

### 测试 4.5: 拒绝错误固件 (N-04)
```
1. 重复 4.2 步骤 (发送 0x0501 进入 OTA 模式)
2. 发送 0x0502 → OK
3. 上位机发送固件, 但前 4 字节用错误的魔术字 (如 00 00 00 00)

4. 设备应:
   OLED: "Bad Magic!" (500ms)
   ← 回复错误帧 (FF EEEE)
   OLED: "Retry OTA..."
   
5. 上位机立刻重发正确的固件
6. 设备接收正确固件 → "OTA OK!"
   ✅ 错误固件被正确拒绝, 正确固件仍然可接收
```

---

## 阶段 5: 完整评测流程测试

### 5.1 从 0 开始的全流程
```
1. 擦除整个芯片 → 烧录 Bootloader (0x08000000)
2. 烧录 APP (0x08011000) 
3. 上电 → 正常启动 → OLED "IDLE"
4. 上位机 19200 → 发送心跳请求 → 设备回复心跳

   ✅ A-01, A-02, A-03 通过

5. 查询版本/时间/ID/波特率/数据/阈值

   ✅ B-01 到 B-06 通过 (RTC 时间仍会返回 1970)
   ⚠️ B-02 仍可能失败 (RTC 需要单独修)

6. 设置变比/阈值 → 重启 → 验证持久化

   ✅ D/F 模块通过 (如果变比/阈值正确)

7. 自动上报

   ✅ H 模块通过

8. 告警

   ✅ I 模块通过

9. 修改波特率 → 立即验证新波特率通信 → 重启 → 再次验证

   ✅ M 模块通过 (现在不重启, 直接切换)

10. 修改 ID → 新 ID 通信 → 重启 → 新 ID 保持

    ✅ L 模块通过

11. OTA 升级

    ✅ N 模块通过
```

---

## 🚨 如果出问题的快速排查

| 现象 | 可能原因 | 排查方法 |
|------|---------|---------|
| Bootloader 编译报错 | proto_bl.c 未加入工程 | 右键 Driver 组 → Add Files |
| 烧录 Bootloader 后 OLED 不亮 | OLED 初始化失败 | 检查 PB8/PB9 接线 |
| OLED 显示 "No APP!" | APP 未烧录或地址不对 | 确认 APP 用的 APP.sct (0x08011000) |
| APP 不启动 (卡在 Bootloader) | APP 向量表损坏 | 检查 APP 编译 0 Error, 0 Warning |
| OTA 模式没进入 | upgrade_flag 未写入 | 检查 APP cmd_handler.c 0x0501 分支 |
| OTA 固件接收失败 | 魔术字不对 | 确认固件前4字节 = 5A A5 C3 3C |
| 写 Flash 失败 | FMC 操作错误 | 检查 BL_FlashWrite 的地址对齐 |
| RS-485 通信失败 | 波特率不匹配 | 上位机和设备波特率必须一致 |
| 心跳帧收到乱码 | printf 混入 | 确认 sys_init.c 用的是 USART1_Config_Baud 而非 USART0_DBG_ReconfigBaud |
