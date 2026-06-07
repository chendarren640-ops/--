#ifndef __BSP_I2C_H
#define __BSP_I2C_H

#include "gd32f4xx.h"
#include <stdint.h>

/*
    软件 I2C 引脚定义
    默认：
    SCL -> PB6
    SDA -> PB7
*/
#define BSP_I2C_GPIO_CLK        RCU_GPIOB
#define BSP_I2C_GPIO_PORT       GPIOB
#define BSP_I2C_SCL_PIN         GPIO_PIN_6
#define BSP_I2C_SDA_PIN         GPIO_PIN_7

/*
    I2C 返回值定义
*/
#define BSP_I2C_OK              0
#define BSP_I2C_ERR_ADDR_W      1
#define BSP_I2C_ERR_REG         2
#define BSP_I2C_ERR_DATA        3
#define BSP_I2C_ERR_ADDR_R      4
#define BSP_I2C_ERR_PARAM       5

void BSP_I2C_Init(void);

uint8_t BSP_I2C_WriteBytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
uint8_t BSP_I2C_ReadBytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);

/*
    兼容你之前代码里可能使用的函数名
*/
#define I2C0_WriteBytes     BSP_I2C_WriteBytes
#define I2C0_ReadBytes      BSP_I2C_ReadBytes

#endif

