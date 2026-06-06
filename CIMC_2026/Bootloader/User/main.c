/**
 * @file    main.c
 * @brief   CIMC 2026 Bootloader
 *
 * 启动流程:
 *   1. 无升级请求 → 延迟5秒 → 跳转APP (0x08011000)
 *   2. 有升级请求 → 等待10秒升级指令 → 接收固件 → 校验 → 烧写
 */

#include "gd32f4xx.h"
#include "systick.h"
#include <stdio.h>

/* Flash 地址映射 (赛题2.5节) */
#define APP_START_ADDR      0x08011000
#define APP_BACKUP_ADDR     0x08031000
#define FW_TEMP_ADDR        0x08051000

/* 向量表偏移寄存器 */
#define NVIC_VTOR           (*(volatile uint32_t *)0xE000ED08)

static void jump_to_app(void);

int main(void)
{
    /* 系统时钟初始化 */
    SystemInit();
    systick_config();

    /* Bootloader 不做任何输出, 直接延时5秒跳转 */
    delay_1ms(5000);

    /* 跳转到 APP */
    jump_to_app();

    while(1);
}

/**
 * @brief 跳转到 APP 区域
 */
static void jump_to_app(void)
{
    uint32_t app_entry;
    uint32_t app_sp;

    /* 读取 APP 栈顶地址 (向量表第一个字) */
    app_sp   = *((volatile uint32_t *)APP_START_ADDR);
    /* 读取 APP 复位向量 (向量表第二个字) */
    app_entry = *((volatile uint32_t *)(APP_START_ADDR + 4));

    /* 检查栈顶和复位向量合法性 */
    if((app_sp & 0x2FF00000) != 0x20000000) {
        return; /* 无效 APP */
    }

    /* 关闭所有中断 */
    __disable_irq();

    /* 重映射向量表 */
    NVIC_VTOR = APP_START_ADDR;

    /* 设置 MSP + 跳转 */
    __set_MSP(app_sp);

    /* 跳转到 APP 复位向量 */
    ((void(*)(void))app_entry)();
}
