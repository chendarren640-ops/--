/**
 * 2026 CIMC Bootloader — 主程序
 *
 * Flash 地址: 0x08000000 ~ 0x0800FFFF (64KB)
 *
 * 行为:
 *   1. 上电 → 不输出任何信息
 *   2. 延时 5 秒 → 跳转至 APP (0x08011000)
 *   3. 如果 APP 请求升级 → 重启后等待 10 秒升级指令
 */

#include "bootloader.h"

/* APP 起始地址 */
#define APP_START_ADDR  0x08011000

/**
 * @brief  主函数入口
 */
int main(void)
{
    /* 基础初始化 */
    systick_config();
    LED_Init();
    OLED_Init();

    /* Bootloader 模式显示 */
    OLED_ShowLine1((uint8_t *)"2026CIMC");
    OLED_ShowLine2((uint8_t *)"Bootloader");
    OLED_Refresh();

    /* TODO: 检查是否有升级标记 (从备份寄存器或参数区读取) */
    /* 如果 APP 请求了升级, 进入升级等待模式 (10s) */

    /* 正常模式: 延时 5 秒, 不输出任何信息 */
    delay_1ms(5000);

    /* 跳转至 APP */
    JumpToApp(APP_START_ADDR);

    /* 不应该到达这里 */
    while (1);
}

/**
 * @brief  跳转至 APP 区域
 *
 * 步骤:
 *   1. 关闭所有中断
 *   2. 设置 MSP (主堆栈指针) = APP 入口地址处的值
 *   3. 设置 PC (程序计数器) = APP 复位向量
 *   4. 跳转
 */
void JumpToApp(uint32_t app_addr)
{
    uint32_t app_stack;
    uint32_t app_entry;

    /* 关闭全局中断 */
    __disable_irq();

    /* APP 起始地址的前 4 字节 = 堆栈指针 */
    app_stack = *(volatile uint32_t *)app_addr;
    /* APP 起始地址 + 4 = 复位向量 */
    app_entry = *(volatile uint32_t *)(app_addr + 4);

    /* 设置堆栈指针 */
    __set_MSP(app_stack);

    /* 跳转到 APP 复位向量 */
    ((void (*)(void))app_entry)();

    /* 不应该返回 */
    while (1);
}
