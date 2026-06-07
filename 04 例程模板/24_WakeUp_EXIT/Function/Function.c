/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Function.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2026/1/30     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"
#include "LED.h"
#include "KEY.h"
#include "usart.h"


/************************* 宏定义 *************************/

/*
 * RCU_MODIFY_4 宏：
 * 用于在切换时钟源前，逐步降低 AHB 总线时钟分频，以避免自移除问题。
 * 参数 __delay：软件延时循环的次数（用于插入等待周期）。
 * 操作流程：
 *   1. 先延时 __delay 个空循环。
 *   2. 将 AHB 分频设为 /2，再延时。
 *   3. 将 AHB 分频设为 /4，再延时。
 *   4. 将 AHB 分频设为 /8，再延时。
 *   5. 将 AHB 分频设为 /16，再延时。
 * 最终 AHB 时钟频率大幅降低，之后可安全切换时钟源。
 */
#define RCU_MODIFY_4(__delay)   do{                                     \
                                    volatile uint32_t i, reg;           \
                                    if(0 != __delay){                   \
                                        /* 插入软件延时 */                \
                                        for(i=0; i<__delay; i++){       \
                                        }                               \
                                        reg = RCU_CFG0;                 \
                                        reg &= ~(RCU_CFG0_AHBPSC);      \
                                        reg |= RCU_AHB_CKSYS_DIV2;      \
                                        /* AHB = 系统时钟 / 2 */         \
                                        RCU_CFG0 = reg;                 \
                                        /* 插入软件延时 */                \
                                        for(i=0; i<__delay; i++){       \
                                        }                               \
                                        reg = RCU_CFG0;                 \
                                        reg &= ~(RCU_CFG0_AHBPSC);      \
                                        reg |= RCU_AHB_CKSYS_DIV4;      \
                                        /* AHB = 系统时钟 / 4 */         \
                                        RCU_CFG0 = reg;                 \
                                        /* 插入软件延时 */                \
                                        for(i=0; i<__delay; i++){       \
                                        }                               \
                                        reg = RCU_CFG0;                 \
                                        reg &= ~(RCU_CFG0_AHBPSC);      \
                                        reg |= RCU_AHB_CKSYS_DIV8;      \
                                        /* AHB = 系统时钟 / 8 */         \
                                        RCU_CFG0 = reg;                 \
                                        /* 插入软件延时 */                \
                                        for(i=0; i<__delay; i++){       \
                                        }                               \
                                        reg = RCU_CFG0;                 \
                                        reg &= ~(RCU_CFG0_AHBPSC);      \
                                        reg |= RCU_AHB_CKSYS_DIV16;     \
                                        /* AHB = 系统时钟 / 16 */        \
                                        RCU_CFG0 = reg;                 \
                                        /* 插入软件延时 */                \
                                        for(i=0; i<__delay; i++){       \
                                        }                               \
                                    }                                   \
                                }while(0)

/************************ 变量定义 ************************/

/* 时钟配置函数声明（内部使用） */
static void rcu_config(void);

/************************ 函数定义 ************************/


/************************************************************
 * Function :       System_Init
 * Comment  :       用于初始化MCU
 * Parameter:       无
 * Return   :       无
 * Author   :       Jialei Zhao
 * Date     :       2026-01-30 V0.1 original
************************************************************/
void System_Init(void)
{
    systick_config();     /* 配置系统滴答定时器，提供延时基准 */

    LED_Init();           /* 初始化LED指示灯 */
    KEY_Init();           /* 初始化按键检测 */
    usart_init();         /* 初始化串口通信 */
}

/************************************************************
 * Function :       UsrFunction
 * Comment  :       用户程序功能: 演示低功耗模式与唤醒后恢复运行
 * Parameter:       无
 * Return   :       无
 * Author   :       Jialei Zhao
 * Date     :       2026-01-30 V0.1 original
************************************************************/
void UsrFunction(void)
{
    /* 使能电源管理单元(PMU)时钟 */
    rcu_periph_clock_enable(RCU_PMU);

    /* 点亮LED1作为开始标记 */
    LED1_ON();

    /* 延时2秒，便于观察 */
    delay_1ms(2000);

    /* 关闭LED1 */
    LED1_OFF();

    /* 进入深度睡眠模式（Deep Sleep）：
     * 使用低功耗LDO、低速驱动，WFI指令触发睡眠。
     * 唤醒后，系统时钟会自动切换到内部16MHz RC振荡器(IRC16M)，
     * 之前的外部晶振(HXTAL)或PLL会被关闭。
     */
    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER , PMU_LOWDRIVER_ENABLE , WFI_CMD);

    /* 从深度睡眠唤醒后，程序从此处继续执行。
     * 此时系统时钟为IRC16M，需要重新配置到240MHz。
     */
    rcu_config();          /* 重新配置时钟到 240MHz */

    /* 主循环：处理串口接收，并控制LED闪烁 */
    while (1)
    {
        usart_recv_buf();                      /* 检测并处理串口接收数据 */

        LED_Toggle(GPIOA , GPIO_PIN_5);        /* 翻转PA5引脚上的LED状态 */

        delay_1ms(500);                        /* 延时500ms，产生1Hz闪烁频率 */
    }
}

/************************************************************
 * Function :       _soft_delay_
 * Comment  :       软件延时函数（忙等）
 * Parameter:       time: 延时时间，单位：ms
 * Return   :       无
 * Author   :       Jialei Zhao
 * Date     :       2026-01-30 V0.1 original
************************************************************/
static void _soft_delay_(uint32_t time)
{
    __IO uint32_t i;          /* volatile 防止优化 */
    /* 循环次数 = time * 10，近似产生 time 毫秒的延时 */
    for (i = 0; i < time * 10; i++)
    {
    }
}

/************************************************************
 * Function :       rcu_config
 * Comment  :       用于配置系统时钟至240MHz
 * Parameter:       无
 * Return   :       无
 * Author   :       Jialei Zhao
 * Date     :       2026-01-30 V0.1 original
 * 注意：本函数在从深度睡眠唤醒后调用，用于恢复高速时钟。
 * 步骤：
 *   1. 调用 RCU_MODIFY_4 宏降低 AHB 时钟，避免时钟突变。
 *   2. 切换系统时钟源为 HXTAL，并复位 RCU 外设（除时钟源外）。
 *   3. 使能 HXTAL 并等待稳定。
 *   4. 配置电源（LDOVS 位使能 LDO 升压）。
 *   5. 设置 AHB/APB1/APB2 预分频。
 *   6. 配置主 PLL 参数（输入25M，N=480，P=2，Q=10），输出 240MHz。
 *   7. 使能 PLL 并等待就绪。
 *   8. 开启高驱动模式(HDEN)，等待稳定后选择高驱动(HDS)。
 *   9. 将系统时钟切换到 PLLP 输出。
************************************************************/
static void rcu_config(void)
{
    uint32_t timeout = 0U;      /* 超时计数器 */
    uint32_t stab_flag = 0U;    /* 稳定标志 */

    /*
     * 强烈建议包含此宏，以避免由于自移除引起的问题。
     * 通过逐步增大 AHB 分频，使系统运行在较低频率，防止时钟切换时出错。
     */
    RCU_MODIFY_4(0x50);

    /* 选择 HXTAL 作为系统时钟源，并复位 RCU 除时钟源外的配置 */
    rcu_system_clock_source_config(RCU_CKSYSSRC_HXTAL);

    /* 插入较长的软件延时，等待时钟切换完成 */
    _soft_delay_(200);

    /* 复位 RCU 配置（不影响当前系统时钟源） */
    rcu_deinit();

    /* 使能外部高速晶振 HXTAL */
    RCU_CTL |= RCU_CTL_HXTALEN;

    /* 等待 HXTAL 稳定，或超时 */
    do
    {
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
    } while ((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

    /* 如果超时仍未稳定，则进入死循环等待（可根据需求调整） */
    if (0U == (RCU_CTL & RCU_CTL_HXTALSTB))
    {
        while (0U == (RCU_CTL & RCU_CTL_HXTALSTB))
        {
        }
    }

    /* 使能 PMU 的 APB1 接口时钟，并设置 LDOVS 以提供充足电压 */
    RCU_APB1EN |= RCU_APB1EN_PMUEN;
    PMU_CTL |= PMU_CTL_LDOVS;

    /* HXTAL 已稳定 */
    /* AHB = SYSCLK / 1 */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB / 2 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    /* APB1 = AHB / 4 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV4;

    /* 配置主 PLL：
     * PLL 输入源 HXTAL (25MHz)
     * PSC = 25  -> 输入分频后为 1MHz
     * PLL_N = 480 -> VCO = 480MHz
     * PLL_P = 2   -> PLLP = VCO/2 = 240MHz
     * PLL_Q = 10  -> PLLQ = VCO/10 = 48MHz
     */
    RCU_PLL = (25U | (480U << 6U) | (((2U >> 1U) - 1U) << 16U) |
        (RCU_PLLSRC_HXTAL) | (10U << 24U));

    /* 使能 PLL */
    RCU_CTL |= RCU_CTL_PLLEN;

    /* 等待 PLL 稳定 */
    while (0U == (RCU_CTL & RCU_CTL_PLLSTB))
    {
    }

    /* 启用高驱动模式，以支持 240MHz 频率 */
    PMU_CTL |= PMU_CTL_HDEN;
    while (0U == (PMU_CS & PMU_CS_HDRF))
    {
    }

    /* 选择高驱动模式 */
    PMU_CTL |= PMU_CTL_HDS;
    while (0U == (PMU_CS & PMU_CS_HDSRF))
    {
    }

    /* 将系统时钟源切换到 PLLP 输出 */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLP;

    /* 等待 PLL 成为系统时钟 */
    while (0U == (RCU_CFG0 & RCU_SCSS_PLLP))
    {
    }
}


/****************************End*****************************/
