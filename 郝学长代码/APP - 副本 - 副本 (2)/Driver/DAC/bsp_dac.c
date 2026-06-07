#include "bsp_dac.h"
#include "systick.h"

/* DAC 基地址和寄存器 */
#define DAC0_BASE           (APB1_BUS_BASE + 0x00007400U)
#define DAC0_CTL0           REG32(DAC0_BASE + 0x00U)
#define DAC0_OUT0_R12DH     REG32(DAC0_BASE + 0x08U)

/* CTL0 位定义 */
#define DAC_CTL0_DEN0       BIT(0)    /* DAC OUT0 使能 */
#define DAC_CTL0_DBOFF0     BIT(1)    /* DAC OUT0 输出缓冲关闭 */

void BSP_DAC_Init(void)
{
    /* 使能 GPIOA 和 DAC 时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_DAC);

    /* 配置 PA4 为模拟模式 */
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);

    /* 直接操作寄存器：禁用缓冲 + 使能 DAC */
    DAC0_CTL0 = DAC_CTL0_DEN0;   /* DEN0=1, DBOFF0=0(缓冲开启), DTEN0=0(无触发) */

    delay_1ms(1);

    /* 初始输出 0V */
    DAC0_OUT0_R12DH = 0;
}

void BSP_DAC_SetVoltage(float voltage)
{
    uint16_t dac_value;

    if(voltage < 0.0f) voltage = 0.0f;
    if(voltage > 3.3f) voltage = 3.3f;

    /* 12位DAC值 = voltage / 3.3 * 4095 */
    dac_value = (uint16_t)(voltage / 3.3f * 4095.0f + 0.5f);

    /* 直接写数据寄存器 */
    DAC0_OUT0_R12DH = dac_value;
}
