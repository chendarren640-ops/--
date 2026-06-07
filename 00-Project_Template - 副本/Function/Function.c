/************************* 头文件 *************************/
#include "Function.h"
#include "gd30ad3340.h"
#include "bsp_i2c.h"
#include "systick.h"
#include "pt100.h"

/************************* 宏定义 *************************/
#define USER_GD30AD3340_CONFIG      GD30AD3340_CONFIG_AIN0_SINGLE_4V096_100SPS
#define USER_GD30AD3340_FSR         4.096f
#define USER_PRINT_PERIOD_MS        500u

/************************ 变量定义 ************************/
volatile int16_t g_gd30ad3340_raw = 0;
volatile float g_gd30ad3340_voltage = 0.0f;
volatile float g_pt100_resistance = 0.0f;
volatile float g_pt100_temperature = 0.0f;
volatile uint8_t g_gd30ad3340_ret = 0;

/************************ 函数定义 ************************/
void System_Init(void)
{
    systick_config();         /* 配置 SysTick 定时器，为延时函数提供基准 */
}

void UsrFunction(void)
{
    uint16_t config_reg;
    
    /* 初始化 USART0 */
    BSP_USART0_Init(115200);
    delay_1ms(100);

    /* 打印启动信息 */
    printf("\r\n");
    printf("========================================\r\n");
    printf("GD32F470 + GD30AD3340 PT100 Demo Start\r\n");
    printf("USART0: PA9 TX, PA10 RX, 115200 8N1\r\n");
    printf("I2C: PB6 SCL, PB7 SDA\r\n");
    printf("ADC Addr: 0x%02X\r\n", GD30AD3340_ADDR);
    printf("ADC Config: AIN0 Single, +/-4.096V, 100SPS\r\n");
    printf("ADC FSR: %.3f V\r\n", USER_GD30AD3340_FSR);
    printf("========================================\r\n");

    /* 初始化 I2C */
    BSP_I2C_Init();
    delay_1ms(20);

    /* 初始化 GD30AD3340（需确保 gd30ad3340.c 写入的是 ±4.096V 配置） */
    GD30AD3340_Init();
    delay_1ms(20);

    /* ★ 校验：读取配置寄存器并打印，确认初始化成功且量程正确 */
    if(GD30AD3340_ReadReg16(GD30AD3340_REG_CONFIG, &config_reg) == 0)
    {
        printf("Config register readback: 0x%04X\r\n", config_reg);
        if((config_reg & 0x0600) == GD30AD3340_PGA_4_096V)   // 检查PGA位
            printf("PGA set to +/-4.096V: OK\r\n");
        else
            printf("Warning: PGA not set to 4.096V, check Init()\r\n");
    }
    else
    {
        printf("Failed to read config register!\r\n");
    }

    while(1)
    {
        /* 启动单次转换并获取原始值和电压值 */
        g_gd30ad3340_ret = GD30AD3340_ReadRawVoltageSingle(
                                USER_GD30AD3340_CONFIG,
                                USER_GD30AD3340_FSR,
                                (int16_t *)&g_gd30ad3340_raw,
                                (float *)&g_gd30ad3340_voltage);

        if(g_gd30ad3340_ret == 0)
        {
            /* 电压 -> PT100 电阻 */
            g_pt100_resistance = PT100_VoltageToResistance(g_gd30ad3340_voltage);
            /* PT100 电阻 -> 温度 */
            g_pt100_temperature = PT100_ResistanceToTemperature(g_pt100_resistance);

            printf("OK | Raw: %d | Voltage: %.6f V | R: %.3f ohm | T: %.2f C\r\n",
                   g_gd30ad3340_raw,
                   g_gd30ad3340_voltage,
                   g_pt100_resistance,
                   g_pt100_temperature);
        }
        else
        {
            printf("ERR | GD30AD3340 read failed, ret = %d\r\n", g_gd30ad3340_ret);
        }

        delay_1ms(USER_PRINT_PERIOD_MS);
    }
}

