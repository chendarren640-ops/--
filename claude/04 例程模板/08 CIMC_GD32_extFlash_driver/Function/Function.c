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
#define  SFLASH_ID                     0xC84013
#define BUFFER_SIZE                    256
#define TX_BUFFER_SIZE                 BUFFER_SIZE
#define RX_BUFFER_SIZE                 BUFFER_SIZE
#define  FLASH_WRITE_ADDRESS           0x000000
#define  FLASH_READ_ADDRESS            FLASH_WRITE_ADDRESS

/************************ 变量定义 ************************/
uint32_t flash_id = 0;
uint8_t  tx_buffer[TX_BUFFER_SIZE];
uint8_t  rx_buffer[TX_BUFFER_SIZE];
uint16_t i = 0;
uint8_t  is_successful = 0;

/************************ 函数定义 ************************/
ErrStatus memory_compare(uint8_t* src,uint8_t* dst,uint16_t length);


/************************************************************ 
 * Function :       System_Init
 * Comment  :       用于初始化MCU
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/

void System_Init(void)
{
	systick_config();     // 时钟配置
	
}

/************************************************************ 
 * Function :       UsrFunction
 * Comment  :       用户程序功能: 外部Flash读写
 * Parameter:       null
 * Return   :       null
 * Author   :       Jianchuan Wang
 * Date     :       2025-02-30 V0.1 original
************************************************************/

uint8_t string[30]={"this is flash test"};

void UsrFunction(void)
{
	LED_Init();
	gd_eval_com_init();
	
	 /* configure SPI0 GPIO and parameter */
    spi_flash_init();
	
	 /* get flash id */
    flash_id = spi_flash_read_id();
	printf("\n\rThe Flash_ID:0x%X\n\r",flash_id);
	
	/* flash id is correct */
    if(SFLASH_ID == flash_id)
	{
		printf("\n\r\n\r******************************erases flash sector*************************\n\r\n\r");
        /* erases the specified flash sector */
        spi_flash_sector_erase(FLASH_WRITE_ADDRESS);
		
		
		 printf("\n\r\n\r******************************Write to tx_buffer:*************************\n\r\n\r");
		/* printf tx_buffer value */
        for(i = 0; i < BUFFER_SIZE; i ++){
            tx_buffer[i] = i;
            printf("0x%02X ",tx_buffer[i]);

            if(15 == i%16)
                printf("\n\r");
        }
		
        /* write tx_buffer data to the flash */ 
        spi_flash_buffer_write(tx_buffer,FLASH_WRITE_ADDRESS,TX_BUFFER_SIZE);

        delay_1ms(10);

		printf("\n\r\n\r******************************Read from tx_buffer:*************************\n\r\n\r");
        /* read a block of data from the flash to rx_buffer */
        spi_flash_buffer_read(rx_buffer,FLASH_READ_ADDRESS,RX_BUFFER_SIZE);
        /* printf rx_buffer value */
        for(i = 0; i <= 255; i ++){
            printf("0x%02X ", rx_buffer[i]);
            if(15 == i%16)
            printf("\n\r");
        }
		
		/*比对读出和写入的数据*/
        if(ERROR == memory_compare(tx_buffer,rx_buffer,256)){
            printf("Err:Data Read and Write aren't Matching.\n\r");
            is_successful = 1;
        }
		
        /* spi flash test passed */
        if(0 == is_successful){
			gpio_bit_set(GPIOA, GPIO_PIN_6);
            printf("\n\rSPI-GD25Q40ESIGR Test Passed!\n\r");
        }else{
			/* spi flash read id fail */
			printf("\n\rSPI Flash: Read ID Fail!\n\r");
		}
		
		
	
		spi_flash_buffer_erase(0,18);		//从0地址处，擦除18个长度
		spi_flash_buffer_write(string,0,18);
		spi_flash_buffer_read(rx_buffer,0,18);
		
		
		printf("String variable reading\r\n");
        /* printf rx_buffer value */
        for(i = 0; i < 18; i ++){
            printf("%c", rx_buffer[i]);
        }
	}
	
	
	while(1)
	{
		gpio_bit_set(GPIOA, GPIO_PIN_5); 
		delay_1ms(500);
		gpio_bit_reset(GPIOA, GPIO_PIN_5);  
		delay_1ms(500);
//		rtc_show_time();
	
		
	
	}
}

/*!
    \brief      memory compare function
    \param[in]  src: source data pointer
    \param[in]  dst: destination data pointer
    \param[in]  length: the compare data length
    \param[out] none
    \retval     ErrStatus: ERROR or SUCCESS
*/
ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length) 
{
    while(length --){
        if(*src++ != *dst++)
            return ERROR;
    }
    return SUCCESS;
}

/****************************End*****************************/

