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
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"

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
uint16_t i = 0, count, result = 0;
uint8_t  is_successful = 0;

FIL fdst;
FATFS fs;
UINT br, bw;
//BYTE textfilebuffer[2048] = "GD32MCU FATFS TEST!\r\n";
BYTE buffer[128];
BYTE filebuffer[128];

/************************ 函数定义 ************************/
ErrStatus memory_compare(uint8_t* src,uint8_t* dst,uint16_t length);
void nvic_config(void);
void write_file(void);

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
 * Comment  :       用户程序功能
 * Parameter:       null
 * Return   :       null
 * Author   :       Liu Tao @ GigaDevice
 * Date     :       2025-05-10 V0.1 original
************************************************************/

void UsrFunction(void)
{
	uint16_t k = 5;
	DSTATUS stat = 0;
	nvic_config();		//配置中断控制器
	
	LED_Init();
	gd_eval_com_init(); //串口初始化

	do
	{
		stat = disk_initialize(0); 			//初始化SD卡（设备号0）,物理驱动器编号,每个物理驱动器（如硬盘、U 盘等）通常都被分配一个唯一的编号。
	}while((stat != 0) && (--k));			//如果初始化失败，重试最多k次。
    
    printf("SD Card disk_initialize:%d\r\n",stat);
    f_mount(0, &fs);						 //挂载SD卡的文件系统（设备号0）。
    printf("SD Card f_mount:%d\r\n",stat);

	if(RES_OK == stat)						 //返回挂载结果（FR_OK 表示成功）。
	{        
        printf("\r\nSD Card Initialize Success!\r\n");
	 
        result = f_open(&fdst, "0:/FATFS.TXT", FA_CREATE_ALWAYS | FA_WRITE);		//在SD卡上创建文件FATFS.TXT。
	 
		write_file();	//调用写文件

		//result = f_write(&fdst, textfilebuffer, sizeof(textfilebuffer), &bw); 	//将textfilebuffer中的数据写入文件。
		result = f_write(&fdst, filebuffer, sizeof(filebuffer), &bw);				//将filebuffer中的数据写入文件。
        
		/**********检查写入结果 begin****************/
		if(FR_OK == result)		
                printf("FATFS FILE write Success!\r\n");
        else
		{
                printf("FATFS FILE write failed!\r\n");
        }
		/**********检查写入结果 end****************/
		
        f_close(&fdst);//关闭文件
		
		
        f_open(&fdst, "0:/FATFS.TXT", FA_OPEN_EXISTING | FA_READ);	//以只读方式重新打开文件
        br = 1;
		
		/**********循环读取文件内容 begin****************/
        for(;;)
		{
			// 清空缓冲区
            for (count=0; count<128; count++)
			{
				buffer[count]=0;
			}
			// 读取文件内容到buffer
            result = f_read(&fdst, buffer, sizeof(buffer), &br);
            if ((0 == result)|| (0 == br))
			{
                break;
			}
        }
		/**********循环读取文件内容 end****************/
		
		// 比较读取的内容与写入的内容是否一致
        if(SUCCESS == memory_compare(buffer, filebuffer, 128))
		{
			printf("FATFS Read File Success!\r\nThe content is:%s\r\n",buffer);
		}
        else
		{
            printf("FATFS FILE read failed!\n");            
        }
         f_close(&fdst);//关闭文件
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


void nvic_config(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);	// 设置中断优先级分组
    nvic_irq_enable(SDIO_IRQn, 0, 0);					// 使能SDIO中断，优先级为0
}

/* 通过串口输入写入文件 */
void write_file(void)
{
    printf("Input data (press Enter to save):\r\n");
    
    uint16_t index = 0;    
    while(1){
        if(usart_flag_get(USART0, USART_FLAG_RBNE) != RESET){
            char ch = usart_data_receive(USART0); 						// 如果接收缓冲区非空，从USART0接收一个字符
            if(ch == '\r'){												// 检查接收到的字符是否为回车键（'\r'）
                filebuffer[index] = '\0';  								// 如果是回车键，在当前位置添加字符串结束符 '\0'
                break;													// 跳出循环，结束数据接收
            }
            filebuffer[index++] = ch;        							// 存储接收到的字符    
            if(index >= sizeof(filebuffer)-1) break;					// 如果缓冲区满则结束接收
        }
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

