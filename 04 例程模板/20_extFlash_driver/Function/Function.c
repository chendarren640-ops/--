/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：Function.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"
#include "LED.h"
#include "RTC.h"
#include "USART.h"
#include "SPI_FLASH.h"

/************************* 宏定义 *************************/
#define  SFLASH_ID                     0xC84013   /* GD25Q40ESIGR 的制造商和设备ID */
#define BUFFER_SIZE                    256        /* 测试缓冲区大小 */
#define TX_BUFFER_SIZE                 BUFFER_SIZE
#define RX_BUFFER_SIZE                 BUFFER_SIZE
#define  FLASH_WRITE_ADDRESS           0x000000   /* 写入/擦除的起始地址 */
#define  FLASH_READ_ADDRESS            FLASH_WRITE_ADDRESS

/************************ 变量定义 ************************/
uint32_t flash_id = 0;                 /* 存放读取的Flash ID */
uint8_t  tx_buffer[TX_BUFFER_SIZE];    /* 发送(写入)缓冲区 */
uint8_t  rx_buffer[TX_BUFFER_SIZE];    /* 接收(读出)缓冲区 */
uint16_t i = 0;                        /* 通用循环变量 */
uint8_t  is_successful = 0;            /* 测试成功标志，0表示成功，非0表示失败 */

/************************ 函数声明 ************************/
ErrStatus memory_compare(uint8_t* src,uint8_t* dst,uint16_t length);


/************************************************************ 
 * Function :       System_Init
 * Comment  :       系统初始化
 * Parameter:       无
 * Return   :       无
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/
void System_Init(void)
{
    /* 配置系统滴答定时器，为延时等功能提供时基 */
    systick_config();
}

/************************************************************ 
 * Function :       UsrFunction
 * Comment  :       用户功能主函数，测试SPI Flash读写
 * Parameter:       无
 * Return   :       无
 * Author   :       Jianchuan Wang
 * Date     :       2025-02-30 V0.1 original
************************************************************/
uint8_t string[30]={"this is flash test"};   /* 用于写入Flash的字符串示例 */

void UsrFunction(void)
{
    /* 初始化LED指示灯 */
    LED_Init();
    /* 初始化串口0，以便使用printf输出调试信息 */
    gd_eval_com_init();

    /* 初始化SPI接口和Flash GPIO */
    spi_flash_init();

    /* 读取Flash器件的ID */
    flash_id = spi_flash_read_id();
    printf("\n\rThe Flash_ID:0x%X\n\r",flash_id);

    /* 如果ID匹配，继续读写测试 */
    if(SFLASH_ID == flash_id)
    {
        printf("\n\r\n\r******************************erases flash sector*************************\n\r\n\r");
        /* 擦除待写入的Flash扇区 */
        spi_flash_sector_erase(FLASH_WRITE_ADDRESS);

        printf("\n\r\n\r******************************Write to tx_buffer:*************************\n\r\n\r");
        /* 填充发送缓冲区并打印 */
        for(i = 0; i < BUFFER_SIZE; i ++){
            tx_buffer[i] = i;                     /* 写入0~255的测试数据 */
            printf("0x%02X ",tx_buffer[i]);

            if(15 == i%16)                         /* 每16个数据换行 */
                printf("\n\r");
        }

        /* 将发送缓冲区的内容写入Flash */
        spi_flash_buffer_write(tx_buffer,FLASH_WRITE_ADDRESS,TX_BUFFER_SIZE);

        delay_1ms(10);

        printf("\n\r\n\r******************************Read from tx_buffer:*************************\n\r\n\r");
        /* 从Flash读出数据到接收缓冲区 */
        spi_flash_buffer_read(rx_buffer,FLASH_READ_ADDRESS,RX_BUFFER_SIZE);
        /* 打印接收缓冲区的内容 */
        for(i = 0; i <= 255; i ++){
            printf("0x%02X ", rx_buffer[i]);
            if(15 == i%16)
                printf("\n\r");
        }

        /* 比较发送和接收缓冲区，验证数据是否正确 */
        if(ERROR == memory_compare(tx_buffer,rx_buffer,256)){
            printf("Err:Data Read and Write aren't Matching.\n\r");
            is_successful = 1;                     /* 标记为失败 */
        }

        /* 根据测试结果给出提示 */
        if(0 == is_successful){
            gpio_bit_set(GPIOA, GPIO_PIN_6);      /* PA6置高，可能是点亮测试通过指示灯 */
            printf("\n\rSPI-GD25Q40ESIGR Test Passed!\n\r");
        }else{
            /* 读取ID失败或数据不匹配时的提示 */
            printf("\n\rSPI Flash: Read ID Fail!\n\r");
        }

        /* 额外测试：向Flash写入一个字符串并读回验证 */
        spi_flash_buffer_erase(0,18);              /* 擦除地址0开始的18个字节 */
        spi_flash_buffer_write(string,0,18);       /* 写入字符串 */
        spi_flash_buffer_read(rx_buffer,0,18);     /* 读回数据 */

        printf("String variable reading\r\n");
        /* 以字符形式打印读回的字符串 */
        for(i = 0; i < 18; i ++){
            printf("%c", rx_buffer[i]);
        }
    }

    /* 主循环，保持程序运行并闪烁指示灯 */
    while(1)
    {
        gpio_bit_set(GPIOA, GPIO_PIN_5);           /* PA5输出高电平 */
        delay_1ms(500);
        gpio_bit_reset(GPIOA, GPIO_PIN_5);         /* PA5输出低电平 */
        delay_1ms(500);
//      rtc_show_time();                            /* 若需要可取消注释，显示RTC时间 */
    }
}

/*!
    \brief      内存比较函数
    \param[in]  src: 源数据指针
    \param[in]  dst: 目标数据指针
    \param[in]  length: 比较的数据长度
    \param[out] 无
    \retval     返回ERROR表示不匹配，SUCCESS表示完全一致
*/
ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length) 
{
    while(length --){
        /* 逐字节比较，一旦发现不同立即返回错误 */
        if(*src++ != *dst++)
            return ERROR;
    }
    return SUCCESS;
}

/****************************End*****************************/

