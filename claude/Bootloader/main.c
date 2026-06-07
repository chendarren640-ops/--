/**
 * 2026 CIMC Bootloader — 主程序 (OTA 固件升级)
 *
 * Flash 地址: 0x08000000 ~ 0x0800FFFF (64KB)
 *
 * 行为:
 *   正常模式: 上电 → "Bootloader" 5s → 跳转 APP (0x08011000)
 *   升级模式: APP 请求升级 → 重启 → 等待 0x0502 → 接收固件 → 校验 → 搬运 → 跳转
 *
 * 赛题 N 模块评测要点 (18分):
 *   N-01: APP 收到 0x0501 → 写 upgrade_flag → 重启进入 Bootloader
 *   N-02: Bootloader 检测 upgrade_flag → 等待 0x0502 命令
 *   N-03: 接收固件切片 (256 字节/片), 校验魔术字 5A A5 C3 3C
 *   N-04: 错误固件 (魔术字不匹配) → 回复错误帧
 *   N-05: 正确固件 → 暂存区搬运到 APP 区
 *   N-06: 刷回 APP 后, APP 不响应升级报文 (验证固件不带升级功能)
 */

#include "bootloader.h"
#include "proto_bl.h"
#include "usart1_bl.h"

/* ========== 读取参数区 ========== */
static void ReadParam(uint16_t *dev_id, uint8_t *baud_code, uint8_t *upgrade_flag)
{
    ParamBlock_t *param = (ParamBlock_t *)PARAM_ADDR;
    *dev_id       = param->device_id;
    *baud_code    = param->baud_code;
    *upgrade_flag = param->upgrade_flag;
}

/* ========== CRC32 (与 APP flash_param.c 一致) ========== */
static uint32_t CalcCRC32(uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (crc & 1 ? 0xEDB88320 : 0);
    }
    return ~crc;
}

/* ========== 清除升级标记 ========== */
static void ClearUpgradeFlag(void)
{
    uint32_t buf[sizeof(ParamBlock_t) / 4];

    /* 1. 先读出当前参数 (擦除前!) */
    uint32_t *src = (uint32_t *)PARAM_ADDR;
    for (int i = 0; i < sizeof(ParamBlock_t) / 4; i++)
        buf[i] = src[i];

    /* 2. 修改 upgrade_flag, 重算 CRC32 */
    ((ParamBlock_t *)buf)->upgrade_flag = 0;
    ((ParamBlock_t *)buf)->crc32 = CalcCRC32((uint8_t *)buf, sizeof(ParamBlock_t) - 4);

    /* 3. 擦除 + 写回 */
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_page_erase(PARAM_ADDR);
    while (fmc_flag_get(FMC_FLAG_BUSY) != RESET);
    fmc_flag_clear(FMC_FLAG_END);
    for (int i = 0; i < sizeof(ParamBlock_t) / 4; i++) {
        fmc_word_program(PARAM_ADDR + i * 4, buf[i]);
        while (fmc_flag_get(FMC_FLAG_BUSY) != RESET);
        fmc_flag_clear(FMC_FLAG_END);
    }
    fmc_lock();
}

/* ========== 闪存擦除与写入 ========== */
void BL_FlashErase(uint32_t addr, uint32_t page_count)
{
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    for (uint32_t i = 0; i < page_count; i++) {
        fmc_page_erase(addr + i * FLASH_PAGE_SIZE);
        while (fmc_flag_get(FMC_FLAG_BUSY) != RESET); /* 等待擦除完成 */
        fmc_flag_clear(FMC_FLAG_END);
    }
    fmc_lock();
}

void BL_FlashWrite(uint32_t addr, uint8_t *data, uint32_t len)
{
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    /* 按字写入, 不足 4 字节对齐部分补齐 0xFF */
    uint32_t words = (len + 3) / 4;
    uint32_t *src  = (uint32_t *)data;
    for (uint32_t i = 0; i < words; i++) {
        uint32_t w = 0xFFFFFFFF;
        if (i < len / 4) {
            w = src[i];
        } else if (i == len / 4 && len % 4 != 0) {
            w = 0xFFFFFFFF;
            memcpy(&w, &data[i * 4], len % 4);
        }
        fmc_word_program(addr + i * 4, w);
        while (fmc_flag_get(FMC_FLAG_BUSY) != RESET); /* 等待写入完成 */
        fmc_flag_clear(FMC_FLAG_END);
    }
    fmc_lock();
}

/* ================================================================
 * OTA 固件接收 — 从 ASCII 十六进制流中接收固件数据
 *
 * 协议: 上位机发送 ASCII 十六进制原始数据流 (不含帧封装)
 *       每个字节展开为 2 个 ASCII 十六进制字符
 *       数据结束后上位机停止发送, 靠超时检测结束
 *
 * 第 1 阶段: 接收所有 ASCII 十六进制数据并转换为二进制
 * 第 2 阶段: 校验魔术字 5A A5 C3 3C
 * 第 3 阶段: 写入暂存区 (0x08051000)
 * ================================================================ */
#define STAGING_RAM_BUF_SIZE  (128 * 1024)  /* 暂存 RAM (最大 128KB) — GD32F470 有 256KB+ SRAM */

/**
 * @brief 从 USART1 缓冲区读取 ASCII 十六进制数据流, 转为二进制
 *
 * 上位机连续发送 ASCII 十六进制字符, 如 "A5B6C3D4..."
 * 每对字符 = 1 字节, 持续接收直到 3 秒无数据
 *
 * @param bin_buf  输出缓冲区 (二进制)
 * @param max_len  最大字节数
 * @param dev_id   设备 ID (用于报错)
 * @return 接收的二进制字节数 (0 = 错误)
 */
static uint32_t RecvFirmwareData(uint8_t *bin_buf, uint32_t max_len, uint16_t dev_id)
{
    uint32_t bin_cnt = 0;
    uint32_t no_data_start = 0;

    /* 清空 USART 缓冲区的残留数据 */
    USART1_Flush();

    /* OLED 提示 */
    OLED_ShowLine2((uint8_t *)"RECV FW...");
    OLED_Refresh();

    /* 接收循环: 3 秒无数据 = 传输结束 (二进制固件, 每字节直接用) */
    while (1) {
        if (USART1_Available() == 0) {
            if (no_data_start == 0) no_data_start = bl_tick;
            if ((bl_tick - no_data_start) > OTA_DATA_TIMEOUT) break;
            delay_1ms(10);
            continue;
        }
        no_data_start = 0;

        if (bin_cnt < max_len) {
            bin_buf[bin_cnt++] = USART1_ReadByte();  /* 直接收二进制, 不转 ASCII hex */
        } else {
            USART1_ReadByte();  /* 超出缓冲区, 丢弃 */
        }
    }

    /* ========== 校验魔术字 ========== */
    if (bin_cnt < 4) {
        /* 数据太少 → 错误 */
        OLED_ShowLine2((uint8_t *)"FW TOO SMALL");
        OLED_Refresh();
        BL_SendError(dev_id);
        return 0;
    }

    if (bin_buf[0] != FW_MAGIC1 || bin_buf[1] != FW_MAGIC2 ||
        bin_buf[2] != FW_MAGIC3 || bin_buf[3] != FW_MAGIC4) {
        /* 魔术字不匹配 → 拒绝错误固件 (N-04) */
        OLED_ShowLine2((uint8_t *)"BAD MAGIC!");
        OLED_Refresh();
        delay_1ms(500);
        BL_SendError(dev_id);
        return 0;
    }

    return bin_cnt;
}

/* ================================================================
 * OTA 升级模式主流程
 * ================================================================ */
static void EnterOTAMode(uint16_t dev_id, uint8_t baud_code)
{
    uint32_t baud = (baud_code == 14) ? 115200UL : 19200UL;

    /* 切换 USART1 为正确的波特率 */
    usart_disable(USART1);
    nvic_irq_disable(USART1_IRQn);
    USART1_BL_Init(baud);

    OLED_ShowLine2((uint8_t *)"OTA MODE");
    OLED_Refresh();

    /* 发送心跳, 通知上位机 Bootloader 已就绪 */
    delay_1ms(500);
    BL_SendHeartbeat(dev_id);

    /* ========== 等待 0x0502 命令 (超时 10s) ========== */
    OLED_ShowLine2((uint8_t *)"WAIT 0502...");
    OLED_Refresh();

    uint8_t  got_cmd = 0;
    uint32_t cmd_deadline = bl_tick + OTA_CMD_TIMEOUT;

    while (bl_tick < cmd_deadline) {
        if (USART1_Available() == 0) {
            delay_1ms(10);
            continue;
        }

        /* 尝试解析帧 */
        uint8_t frame[300];
        uint8_t result = BL_ParseHexFrame(frame, sizeof(frame));

        if (result != FRAME_OK) continue;

        /* 检查是否是 0x0502 命令 */
        uint16_t frame_id  = ((uint16_t)frame[2] << 8) | frame[3];
        uint8_t  frame_tt  = frame[4];
        uint16_t frame_cmd = ((uint16_t)frame[5] << 8) | frame[6];

        /* ID 不匹配且非广播 → 忽略 */
        if (frame_id != 0xFFFF && frame_id != dev_id) continue;

        /* 心跳请求 → 回复心跳 */
        if (frame_tt == 0x05 && frame_cmd == 0xFFFF) {
            BL_SendHeartbeat(dev_id);
            continue;
        }

        /* 0x0502 固件升级请求 */
        if (frame_tt == 0x01 && frame_cmd == 0x0502) {
            got_cmd = 1;
            break;
        }

        /* 未知帧 → 错误应答 */
        BL_SendError(dev_id);
    }

    if (!got_cmd) {
        /* 超时 → 直接跳转 APP */
        OLED_ShowLine2((uint8_t *)"OTA TIMEOUT");
        OLED_Refresh();
        delay_1ms(1000);
        JumpToApp(APP_START_ADDR);
        return;
    }

    /* ========== 回复 OK to 0x0502, 准备接收固件数据 ========== */
    BL_SendOK(0x0502, dev_id);
    delay_1ms(200);

    /* ========== 接收固件 (上位机在 0x0502 OK 后 ~500ms 开始下发 bin) ========== */
    /* 使用 SRAM 暂存 (GD32F470 有足够的 SRAM) */
    static uint8_t fw_buf[STAGING_RAM_BUF_SIZE] __attribute__((aligned(4)));

    uint32_t fw_size = RecvFirmwareData(fw_buf, STAGING_RAM_BUF_SIZE, dev_id);

    if (fw_size == 0) {
        /* 魔术字错误 — 已在 RecvFirmwareData 中发送错误帧 */
        /* 重新进入等待模式 (上位机会重发正确固件) */
        OLED_ShowLine2((uint8_t *)"RETRY OTA...");
        OLED_Refresh();
        delay_1ms(2000);

        /* 重新等待固件数据 */
        USART1_Flush();
        fw_size = RecvFirmwareData(fw_buf, STAGING_RAM_BUF_SIZE, dev_id);
        if (fw_size == 0) {
            OLED_ShowLine2((uint8_t *)"FW FAIL!");
            OLED_Refresh();
            delay_1ms(2000);
            JumpToApp(APP_START_ADDR);
            return;
        }
    }

    /* ========== 等待 0x0503 执行升级命令 ========== */
    {
        OLED_ShowLine2((uint8_t *)"WAIT 0503...");
        OLED_Refresh();

        uint8_t  got_exec = 0;
        uint32_t exec_deadline = bl_tick + OTA_CMD_TIMEOUT;
        while (bl_tick < exec_deadline) {
            if (USART1_Available() == 0) { delay_1ms(10); continue; }
            uint8_t frame[300];
            if (BL_ParseHexFrame(frame, sizeof(frame)) != FRAME_OK) continue;
            uint16_t fid = ((uint16_t)frame[2]<<8)|frame[3];
            uint8_t  ftt = frame[4];
            uint16_t fcmd = ((uint16_t)frame[5]<<8)|frame[6];
            if (fid != 0xFFFF && fid != dev_id) continue;
            if (ftt == 0x05 && fcmd == 0xFFFF) { BL_SendHeartbeat(dev_id); continue; }
            if (ftt == 0x01 && fcmd == 0x0503) { got_exec = 1; break; }
            BL_SendError(dev_id);
        }
        if (!got_exec) {
            OLED_ShowLine2((uint8_t *)"NO 0503!");
            OLED_Refresh(); delay_1ms(2000);
            JumpToApp(APP_START_ADDR); return;
        }
    }

    /* 回复 OK to 0x0503 */
    BL_SendOK(0x0503, dev_id);
    delay_1ms(200);

    /* ========== 固件写入暂存区 Flash ========== */
    OLED_ShowLine2((uint8_t *)"WRITE FLASH");
    OLED_Refresh();

    /* 擦除暂存区 */
    uint32_t pages = (fw_size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
    BL_FlashErase(STAGING_ADDR, pages);

    /* 写入暂存区 */
    BL_FlashWrite(STAGING_ADDR, fw_buf, fw_size);

    /* 校验写入 */
    uint8_t mismatch = 0;
    for (uint32_t i = 0; i < fw_size; i++) {
        if (*(volatile uint8_t *)(STAGING_ADDR + i) != fw_buf[i]) {
            mismatch = 1;
            break;
        }
    }

    if (mismatch) {
        OLED_ShowLine2((uint8_t *)"FLASH ERR!");
        OLED_Refresh();
        BL_SendError(dev_id);
        delay_1ms(2000);
        JumpToApp(APP_START_ADDR);
        return;
    }

    /* ========== 搬运: 暂存区 → APP 区 (N-05) ========== */
    OLED_ShowLine2((uint8_t *)"COPY TO APP");
    OLED_Refresh();

    /* 擦除 APP 区 */
    uint32_t app_pages = (fw_size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
    BL_FlashErase(APP_START_ADDR, app_pages);

    /* 从暂存区逐字复制到 APP 区 */
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    uint32_t words = (fw_size + 3) / 4;
    for (uint32_t i = 0; i < words; i++) {
        uint32_t w = *(volatile uint32_t *)(STAGING_ADDR + i * 4);
        fmc_word_program(APP_START_ADDR + i * 4, w);
        while (fmc_flag_get(FMC_FLAG_BUSY) != RESET);  /* 等待写入完成 */
        fmc_flag_clear(FMC_FLAG_END);
    }
    fmc_lock();

    /* 校验 APP 区 */
    mismatch = 0;
    for (uint32_t i = 0; i < fw_size; i++) {
        if (*(volatile uint8_t *)(APP_START_ADDR + i) !=
            *(volatile uint8_t *)(STAGING_ADDR + i)) {
            mismatch = 1;
            break;
        }
    }

    if (mismatch) {
        OLED_ShowLine2((uint8_t *)"COPY ERR!");
        OLED_Refresh();
        BL_SendError(dev_id);
        delay_1ms(2000);
        JumpToApp(APP_START_ADDR);
        return;
    }

    /* ========== 升级完成 ========== */
    OLED_ShowLine2((uint8_t *)"OTA OK!");
    OLED_Refresh();
    delay_1ms(1000);

    /* 跳转到新 APP */
    JumpToApp(APP_START_ADDR);
}

void HardFault_Handler(void)
{
    while (1) {
        gpio_bit_toggle(LED_SYS_PORT, LED_SYS_PIN);
        for (volatile uint32_t d = 0; d < 200000; d++);
    }
}

/* ================================================================
 * 主函数
 * ================================================================ */
int main(void)
{
    uint16_t dev_id;
    uint8_t  baud_code;
    uint8_t  upgrade_flag;

    systick_config();
    LED_Init();
    OLED_Init();
    OLED_ShowLine1((uint8_t *)"2026CIMC");

    /* 读取参数区 */
    ReadParam(&dev_id, &baud_code, &upgrade_flag);

    /* ========== 升级模式 ========== */
    if (upgrade_flag == UPGRADE_FLAG_REQ) {
        ClearUpgradeFlag();
        EnterOTAMode(dev_id, baud_code);
        while (1);
    }

    /* ========== 正常启动 ========== */
    OLED_ShowLine2((uint8_t *)"BOOTLOADER");
    OLED_Refresh();

    /* LED 闪烁 3 次验证基础功能 */
    for (int i = 0; i < 3; i++) {
        gpio_bit_set(LED_SYS_PORT, LED_SYS_PIN);
        delay_1ms(500);
        gpio_bit_reset(LED_SYS_PORT, LED_SYS_PIN);
        delay_1ms(500);
    }

    /* 初始化 USART1 (用于倒计时输出 + 接收中断升级指令) */
    {
        uint32_t baud = (baud_code == 14) ? 115200UL : 19200UL;
        USART1_BL_Init(baud);
    }

    /* ========== 倒计时输出 (赛题 N-01 要求) ========== */
    BL_SendString("using command to interrupt start Application\r\n");

    /* 10 秒倒计时: 10→7→4→1, 同时监听上位机中断命令 */
    {
        uint32_t countdown_start = bl_tick;
        int last_report = 10;
        while (1) {
            uint32_t elapsed = bl_tick - countdown_start;
            int remaining = 10 - (int)(elapsed / 1000);
            if (remaining < 0) remaining = 0;

            /* 打印倒计时 (每次减3秒时输出) */
            if (remaining <= 1 && last_report > 1) {
                BL_SendString("wait for start Application (1s)...\r\n");
                last_report = 1;
            } else if (remaining <= 4 && last_report > 4) {
                BL_SendString("wait for start Application (4s)...\r\n");
                last_report = 4;
            } else if (remaining <= 7 && last_report > 7) {
                BL_SendString("wait for start Application (7s)...\r\n");
                last_report = 7;
            }

            if (remaining <= 0) break;

            /* 监听上位机中断命令 (升级请求帧) */
            if (USART1_Available() > 0) {
                uint8_t frame[300];
                uint8_t result = BL_ParseHexFrame(frame, sizeof(frame));
                if (result == FRAME_OK) {
                    uint16_t fid  = ((uint16_t)frame[2]<<8)|frame[3];
                    uint8_t  ftt  = frame[4];
                    uint16_t fcmd = ((uint16_t)frame[5]<<8)|frame[6];
                    if ((fid == 0xFFFF || fid == dev_id) &&
                        ftt == 0x01 && fcmd == 0x0501) {
                        /* 收到升级请求 → 进入升级模式 */
                        BL_SendOK(0x0501, dev_id);
                        delay_1ms(100);
                        EnterOTAMode(dev_id, baud_code);
                        while (1);
                    }
                }
            }
            delay_1ms(100);
        }
    }

    JumpToApp(APP_START_ADDR);

    /* 跳转失败 → 停留 */
    OLED_ShowLine2((uint8_t *)"NO APP!");
    OLED_Refresh();
    while (1) {
        gpio_bit_toggle(LED_SYS_PORT, LED_SYS_PIN);
        for (volatile uint32_t d = 0; d < 200000; d++);
    }
}

/* ================================================================
 * 跳转至 APP
 * ================================================================ */
void JumpToApp(uint32_t app_addr)
{
    uint32_t i;

    /* 1. 停止 USART1 (Bootloader 开启了 USART1 RX 中断) */
    usart_disable(USART1);

    /* 2. 关闭全局中断 + SysTick */
    __disable_irq();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    /* 3. 禁用并清除所有 NVIC 中断 (参考: bl_jump_to_app) */
    for (i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;  /* 禁用所有中断 */
        NVIC->ICPR[i] = 0xFFFFFFFF;  /* 清除所有挂起中断 */
    }

    __DSB();
    __ISB();

    /* 4. 验证 APP 向量表 */
    uint32_t app_stack = *(volatile uint32_t *)app_addr;
    uint32_t app_entry = *(volatile uint32_t *)(app_addr + 4);

    if ((app_stack & 0x2FFC0000UL) != 0x20000000UL ||  /* 允许 256KB SRAM */
        app_entry < 0x08011000UL || app_entry > 0x08080000UL) {
        OLED_ShowLine2((uint8_t *)"NO APP!");
        OLED_Refresh();
        while (1) {
            gpio_bit_toggle(LED_SYS_PORT, LED_SYS_PIN);
            for (volatile uint32_t d = 0; d < 200000; d++);
        }
    }

    /* 5. 设置向量表 + 堆栈 */
    SCB->VTOR = app_addr;
    __set_MSP(app_stack);

    /* 6. 恢复中断, 跳转到 APP 复位向量 */
    __enable_irq();
    ((void (*)(void))app_entry)();

    while (1);
}
