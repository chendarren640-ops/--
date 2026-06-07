#include "bsp_i2c.h"

/*
    软件 I2C 延时。
    如果 I2C 通信不稳定，可以适当增大循环次数。
*/
static void I2C_Delay(void)
{
    volatile uint32_t i;

    for(i = 0; i < 80; i++)
    {
        __NOP();
    }
}

/*
    SCL 输出高低电平
*/
static void I2C_SCL_H(void)
{
    gpio_bit_set(BSP_I2C_GPIO_PORT, BSP_I2C_SCL_PIN);
}

static void I2C_SCL_L(void)
{
    gpio_bit_reset(BSP_I2C_GPIO_PORT, BSP_I2C_SCL_PIN);
}

/*
    SDA 输出高低电平
    因为 SDA 是开漏输出：
    写 1 表示释放 SDA，由外部/内部上拉拉高
    写 0 表示主动拉低 SDA
*/
static void I2C_SDA_H(void)
{
    gpio_bit_set(BSP_I2C_GPIO_PORT, BSP_I2C_SDA_PIN);
}

static void I2C_SDA_L(void)
{
    gpio_bit_reset(BSP_I2C_GPIO_PORT, BSP_I2C_SDA_PIN);
}

/*
    读取 SDA 电平
*/
static uint8_t I2C_SDA_READ(void)
{
    return gpio_input_bit_get(BSP_I2C_GPIO_PORT, BSP_I2C_SDA_PIN);
}

/*
    I2C 起始信号
    SDA 在 SCL 为高时，从高变低
*/
static void I2C_Start(void)
{
    I2C_SDA_H();
    I2C_SCL_H();
    I2C_Delay();

    I2C_SDA_L();
    I2C_Delay();

    I2C_SCL_L();
    I2C_Delay();
}

/*
    I2C 停止信号
    SDA 在 SCL 为高时，从低变高
*/
static void I2C_Stop(void)
{
    I2C_SDA_L();
    I2C_Delay();

    I2C_SCL_H();
    I2C_Delay();

    I2C_SDA_H();
    I2C_Delay();
}

/*
    主机发送 ACK
*/
static void I2C_SendAck(void)
{
    I2C_SCL_L();
    I2C_SDA_L();
    I2C_Delay();

    I2C_SCL_H();
    I2C_Delay();

    I2C_SCL_L();
    I2C_SDA_H();
    I2C_Delay();
}

/*
    主机发送 NACK
*/
static void I2C_SendNack(void)
{
    I2C_SCL_L();
    I2C_SDA_H();
    I2C_Delay();

    I2C_SCL_H();
    I2C_Delay();

    I2C_SCL_L();
    I2C_Delay();
}

/*
    等待从机 ACK
    返回 0：收到 ACK
    返回 1：未收到 ACK
*/
static uint8_t I2C_WaitAck(void)
{
    uint16_t timeout = 0;

    I2C_SCL_L();
    I2C_SDA_H();
    I2C_Delay();

    I2C_SCL_H();
    I2C_Delay();

    while(I2C_SDA_READ())
    {
        timeout++;

        if(timeout > 500)
        {
            I2C_SCL_L();
            return 1;
        }
    }

    I2C_SCL_L();
    I2C_Delay();

    return 0;
}

/*
    I2C 发送 1 字节
*/
static void I2C_SendByte(uint8_t byte)
{
    uint8_t i;

    for(i = 0; i < 8; i++)
    {
        I2C_SCL_L();

        if(byte & 0x80)
        {
            I2C_SDA_H();
        }
        else
        {
            I2C_SDA_L();
        }

        byte <<= 1;

        I2C_Delay();

        I2C_SCL_H();
        I2C_Delay();

        I2C_SCL_L();
        I2C_Delay();
    }

    I2C_SDA_H();
}

/*
    I2C 读取 1 字节
*/
static uint8_t I2C_ReadByte(void)
{
    uint8_t i;
    uint8_t byte = 0;

    I2C_SDA_H();

    for(i = 0; i < 8; i++)
    {
        byte <<= 1;

        I2C_SCL_L();
        I2C_Delay();

        I2C_SCL_H();
        I2C_Delay();

        if(I2C_SDA_READ())
        {
            byte |= 0x01;
        }

        I2C_SCL_L();
        I2C_Delay();
    }

    return byte;
}

/*
    软件 I2C 初始化
*/
void BSP_I2C_Init(void)
{
    rcu_periph_clock_enable(BSP_I2C_GPIO_CLK);

    /*
        PB6/PB7 配置为开漏输出，上拉。
        软件 I2C 推荐开漏模式。
    */
    gpio_mode_set(BSP_I2C_GPIO_PORT,
                  GPIO_MODE_OUTPUT,
                  GPIO_PUPD_PULLUP,
                  BSP_I2C_SCL_PIN | BSP_I2C_SDA_PIN);

    gpio_output_options_set(BSP_I2C_GPIO_PORT,
                            GPIO_OTYPE_OD,
                            GPIO_OSPEED_50MHZ,
                            BSP_I2C_SCL_PIN | BSP_I2C_SDA_PIN);

    I2C_SCL_H();
    I2C_SDA_H();
    I2C_Delay();
}

/*
    向 I2C 设备寄存器连续写入多个字节

    dev_addr：7 位 I2C 地址，例如 GD30AD3340 为 0x48
    reg_addr：寄存器地址
    data：待写入数据
    len：写入长度

    写入流程：
    START
    设备地址 + 写
    寄存器地址
    数据 0
    数据 1
    ...
    STOP
*/
uint8_t BSP_I2C_WriteBytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    uint16_t i;

    if((data == 0) || (len == 0))
    {
        return BSP_I2C_ERR_PARAM;
    }

    I2C_Start();

    I2C_SendByte((uint8_t)(dev_addr << 1));
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return BSP_I2C_ERR_ADDR_W;
    }

    I2C_SendByte(reg_addr);
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return BSP_I2C_ERR_REG;
    }

    for(i = 0; i < len; i++)
    {
        I2C_SendByte(data[i]);
        if(I2C_WaitAck())
        {
            I2C_Stop();
            return BSP_I2C_ERR_DATA;
        }
    }

    I2C_Stop();

    return BSP_I2C_OK;
}

/*
    从 I2C 设备寄存器连续读取多个字节

    dev_addr：7 位 I2C 地址，例如 GD30AD3340 为 0x48
    reg_addr：寄存器地址
    data：读取数据缓存
    len：读取长度

    读取流程：
    START
    设备地址 + 写
    寄存器地址
    RESTART
    设备地址 + 读
    读取数据 0，发送 ACK
    读取数据 1，发送 ACK
    ...
    最后 1 字节，发送 NACK
    STOP
*/
uint8_t BSP_I2C_ReadBytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    uint16_t i;

    if((data == 0) || (len == 0))
    {
        return BSP_I2C_ERR_PARAM;
    }

    I2C_Start();

    I2C_SendByte((uint8_t)(dev_addr << 1));
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return BSP_I2C_ERR_ADDR_W;
    }

    I2C_SendByte(reg_addr);
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return BSP_I2C_ERR_REG;
    }

    I2C_Start();

    I2C_SendByte((uint8_t)((dev_addr << 1) | 0x01));
    if(I2C_WaitAck())
    {
        I2C_Stop();
        return BSP_I2C_ERR_ADDR_R;
    }

    for(i = 0; i < len; i++)
    {
        data[i] = I2C_ReadByte();

        if(i < (len - 1))
        {
            I2C_SendAck();
        }
        else
        {
            I2C_SendNack();
        }
    }

    I2C_Stop();

    return BSP_I2C_OK;
}

