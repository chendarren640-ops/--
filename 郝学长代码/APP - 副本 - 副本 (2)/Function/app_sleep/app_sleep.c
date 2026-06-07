/*
 * app_sleep.c
 * 深度睡眠模块 (RTC 唤醒 10 秒)
 * 基于 GD32F470 低功耗模式
 */

#include "app_sleep.h"
#include "app_param.h"
#include "bsp_usart_rs485.h"
#include "bsp_rtc.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_pmu.h"
#include "gd32f4xx_rtc.h"
#include <string.h>
#include <stdio.h>

extern void delay_1ms(uint32_t ms);

/* RTC 唤醒定时器标志 (使用库定义的 RTC_FLAG_WT) */
#define RTC_FLAG_WUTF  RTC_FLAG_WT

/* ==================== 时钟恢复 (参考官方示例) ==================== */

#define RCU_MODIFY_4(__delay)   do{                                     \
                                    volatile uint32_t i, reg;           \
                                    if(0 != __delay){                   \
                                        for(i=0; i<__delay; i++){}      \
                                        reg = RCU_CFG0;                 \
                                        reg &= ~(RCU_CFG0_AHBPSC);     \
                                        reg |= RCU_AHB_CKSYS_DIV2;     \
                                        RCU_CFG0 = reg;                 \
                                        for(i=0; i<__delay; i++){}      \
                                        reg = RCU_CFG0;                 \
                                        reg &= ~(RCU_CFG0_AHBPSC);     \
                                        reg |= RCU_AHB_CKSYS_DIV4;     \
                                        RCU_CFG0 = reg;                 \
                                        for(i=0; i<__delay; i++){}      \
                                        reg = RCU_CFG0;                 \
                                        reg &= ~(RCU_CFG0_AHBPSC);     \
                                        reg |= RCU_AHB_CKSYS_DIV8;     \
                                        RCU_CFG0 = reg;                 \
                                        for(i=0; i<__delay; i++){}      \
                                        reg = RCU_CFG0;                 \
                                        reg &= ~(RCU_CFG0_AHBPSC);     \
                                        reg |= RCU_AHB_CKSYS_DIV16;    \
                                        RCU_CFG0 = reg;                 \
                                        for(i=0; i<__delay; i++){}      \
                                    }                                   \
                                }while(0)

static void _soft_delay_(uint32_t time)
{
    __IO uint32_t i;
    for (i = 0; i < time * 10; i++) {}
}

/*
 * 时钟恢复: 睡眠唤醒后重新配置系统时钟到 240MHz
 * 外部晶振 HXTAL = 25MHz, PLL = 25*480/2 = 240MHz
 */
static void app_sleep_rcu_recovery(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    RCU_MODIFY_4(0x50);

    /* 先切换到 HXTAL 作为系统时钟源 */
    rcu_system_clock_source_config(RCU_CKSYSSRC_HXTAL);
    _soft_delay_(200);

    /* 复位 RCU 配置 */
    rcu_deinit();

    /* 使能 HXTAL */
    RCU_CTL |= RCU_CTL_HXTALEN;

    /* 等待 HXTAL 稳定 */
    do {
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
    } while ((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

    if (0U == (RCU_CTL & RCU_CTL_HXTALSTB)) {
        while (0U == (RCU_CTL & RCU_CTL_HXTALSTB)) {}
    }

    /* 使能 PMU 高驱动模式 LDO */
    RCU_APB1EN |= RCU_APB1EN_PMUEN;
    PMU_CTL |= PMU_CTL_LDOVS;

    /* 配置分频 */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV4;

    /* 配置 PLL (25MHz * 480 / 2 = 240MHz) */
    RCU_PLL = (25U | (480U << 6U) | (((2U >> 1U) - 1U) << 16U) |
               (RCU_PLLSRC_HXTAL) | (10U << 24U));

    /* 使能 PLL */
    RCU_CTL |= RCU_CTL_PLLEN;
    while (0U == (RCU_CTL & RCU_CTL_PLLSTB)) {}

    /* 配置高驱动模式 */
    PMU_CTL |= PMU_CTL_HDEN;
    while (0U == (PMU_CS & PMU_CS_HDRF)) {}
    PMU_CTL |= PMU_CTL_HDS;
    while (0U == (PMU_CS & PMU_CS_HDSRF)) {}

    /* 切换到 PLLP 作为主时钟 */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLP;
    while (0U == (RCU_CFG0 & RCU_SCSS_PLLP)) {}
}

/* ==================== 初始化 ==================== */

void APP_Sleep_Init(void)
{
    /* 睡眠模块初始化 (当前无需特殊初始化) */
}

/* ==================== 核心睡眠函数 ==================== */

void APP_Sleep_Enter10s(void)
{
    uint32_t actual_baudrate;

    /* 1. 发送睡眠提示 */
    const char *msg_sleep = "instrument sleep\r\n";
    BSP_RS485_SendData((const uint8_t *)msg_sleep, strlen(msg_sleep));
    while (usart_flag_get(USART1, USART_FLAG_TC) == RESET);

    /* 2. 配置 RTC 唤醒定时器 (10秒) */
    rtc_wakeup_disable();
    rtc_flag_clear(RTC_FLAG_WUTF);
    rtc_wakeup_clock_set(WAKEUP_CKSPRE);
    rtc_wakeup_timer_set(10);
    rtc_wakeup_enable();

    /* 3. 使能 RTC 唤醒中断 */
    nvic_irq_enable(RTC_WKUP_IRQn, 0, 0);

    /* 4. 短延时确保配置生效 */
    delay_1ms(5);

    /* 5. 进入深度睡眠模式 */
    pmu_to_deepsleepmode(PMU_LDO_NORMAL, PMU_LOWDRIVER_DISABLE, WFI_CMD);

    /* ============ 唤醒后 ============ */
    app_sleep_rcu_recovery();               /* 恢复时钟到 240MHz */

    /* 恢复串口 (使用保存的波特率) */
    actual_baudrate = APP_Param_GetBaudrateValue(g_app_param.baudrate);
    BSP_RS485_Init(actual_baudrate);

    /* 发送唤醒提示 */
    const char *msg_wake = "instrument wakeup\r\n";
    BSP_RS485_SendData((const uint8_t *)msg_wake, strlen(msg_wake));
    while (usart_flag_get(USART1, USART_FLAG_TC) == RESET);
}
