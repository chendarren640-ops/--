#include "gd30ad3340.h"
#include "bsp_i2c.h"
#include "systick.h"

/*
    默认满量程电压（应与初始化配置匹配）。
    注意：实际值由初始化时写入的配置决定，此处仅为默认值，用户可覆盖。
*/
#define GD30AD3340_DEFAULT_FSR_VOLTAGE    2.048f

/*
    转换超时时间（ms）。100SPS单次转换约10ms，取30ms有足够余量。
*/
#define GD30AD3340_CONVERSION_TIMEOUT_MS  30u

/**
 * @brief  向 GD30AD3340 的 16 位寄存器写入 16 位数据
 */
uint8_t GD30AD3340_WriteReg16(uint8_t reg, uint16_t value)
{
    uint8_t data[2];
    data[0] = (uint8_t)(value >> 8);
    data[1] = (uint8_t)(value & 0xFF);
    return I2C0_WriteBytes(GD30AD3340_ADDR, reg, data, 2);
}

/**
 * @brief  从 GD30AD3340 的 16 位寄存器读取 16 位数据
 */
uint8_t GD30AD3340_ReadReg16(uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    uint8_t ret;

    if(value == 0)
        return 0xFE;

    ret = I2C0_ReadBytes(GD30AD3340_ADDR, reg, data, 2);
    if(ret != 0)
        return ret;

    *value = ((uint16_t)data[0] << 8) | data[1];
    return 0;
}

/**
 * @brief  GD30AD3340 初始化
 *         ★ 修改：使用扩展量程 ±4.096V，与主循环配置一致（可根据需要修改）
 */
void GD30AD3340_Init(void)
{
    /*
        此处应使用与主循环相同的配置宏。
        如果主循环使用 GD30AD3340_CONFIG_AIN0_SINGLE_4V096_100SPS，则这里也用它。
        为了灵活性，可以改为参数化，但此处直接写死为 4.096V 量程。
    */
    GD30AD3340_WriteReg16(GD30AD3340_REG_CONFIG,
                          GD30AD3340_CONFIG_AIN0_SINGLE_4V096_100SPS);
    delay_1ms(10);   // 等待配置生效
}

/**
 * @brief  启动一次单次转换，并读取原始转换结果
 * @param  config  配置值（不含 OS 位），函数内会自动加上 OS_START
 * @param  raw     存放原始 ADC 结果的指针（有符号 16 位）
 * @return 0: 成功，其他: 错误码
 */
uint8_t GD30AD3340_ReadRawSingle(uint16_t config, int16_t *raw)
{
    uint16_t conv;
    uint8_t ret;
    uint16_t status;
    uint32_t timeout;

    if(raw == 0)
        return 0xFE;

    /* 写入配置 + 启动转换（OS 位置 1） */
    ret = GD30AD3340_WriteReg16(GD30AD3340_REG_CONFIG,
                                config | GD30AD3340_OS_START);
    if(ret != 0)
        return ret;

    /* 等待转换完成：轮询配置寄存器中的 OS 位（转换完成后自动清零） */
    timeout = 0;
    do {
        delay_1ms(1);
        ret = GD30AD3340_ReadReg16(GD30AD3340_REG_CONFIG, &status);
        if(ret != 0)
            return ret;
        timeout++;
    } while((status & GD30AD3340_OS_START) && (timeout < GD30AD3340_CONVERSION_TIMEOUT_MS));

    if(timeout >= GD30AD3340_CONVERSION_TIMEOUT_MS)
        return 0xEE;   // 转换超时

    /* 读取转换结果 */
    ret = GD30AD3340_ReadReg16(GD30AD3340_REG_CONVERSION, &conv);
    if(ret != 0)
        return ret;

    *raw = (int16_t)conv;   // 转换为有符号整数
    return 0;
}

/**
 * @brief  启动一次单次转换，并计算实际电压值
 */
uint8_t GD30AD3340_ReadVoltageSingle(uint16_t config, float fsr, float *voltage)
{
    int16_t raw;
    uint8_t ret;

    if(voltage == 0)
        return 0xFE;

    ret = GD30AD3340_ReadRawSingle(config, &raw);
    if(ret != 0)
        return ret;

    *voltage = ((float)raw * fsr) / 32768.0f;
    return 0;
}

/**
 * @brief  同时获取原始值和电压值
 */
uint8_t GD30AD3340_ReadRawVoltageSingle(uint16_t config, float fsr, int16_t *raw, float *voltage)
{
    uint8_t ret;

    if((raw == 0) || (voltage == 0))
        return 0xFE;

    ret = GD30AD3340_ReadRawSingle(config, raw);
    if(ret != 0)
        return ret;

    *voltage = ((float)(*raw) * fsr) / 32768.0f;
    return 0;
}

/**
 * @brief  使用默认配置快速读取 AIN0 单端的原始值和电压
 * @note   默认使用 ±2.048V 量程（与初始化配置一致）
 */
uint8_t GD30AD3340_ReadAIN0Default(int16_t *raw, float *voltage)
{
    return GD30AD3340_ReadRawVoltageSingle(
        GD30AD3340_CONFIG_AIN0_SINGLE_2V048_100SPS,
        GD30AD3340_DEFAULT_FSR_VOLTAGE,
        raw,
        voltage
    );
}