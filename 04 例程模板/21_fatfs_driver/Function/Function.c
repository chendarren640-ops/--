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
#include "ff.h"             /* FATFS 文件系统核心头文件 */
#include "diskio.h"         /* FATFS 底层磁盘IO接口 */
#include "sdcard.h"         /* SD卡驱动头文件 */

/************************* 宏定义 *************************/
#define  SFLASH_ID                     0xC84013   /* GD25Q40ESIGR 的制造商和设备ID */
#define BUFFER_SIZE                    256        /* 测试缓冲区大小 */
#define TX_BUFFER_SIZE                 BUFFER_SIZE
#define RX_BUFFER_SIZE                 BUFFER_SIZE
#define  FLASH_WRITE_ADDRESS           0x000000   /* Flash写入起始地址 */
#define  FLASH_READ_ADDRESS            FLASH_WRITE_ADDRESS

/************************ 变量定义 ************************/
uint32_t flash_id = 0;                 /* 存放读取的Flash ID */
uint8_t  tx_buffer[TX_BUFFER_SIZE];    /* 发送(写入)缓冲区 */
uint8_t  rx_buffer[RX_BUFFER_SIZE];    /* 接收(读出)缓冲区 */
uint16_t i = 0, count, result = 0;     /* 通用循环变量、计数器及操作结果 */
uint8_t  is_successful = 0;            /* 操作成功标志 */

FIL fdst;                              /* FATFS 文件对象，用于文件操作 */
FATFS fs;                              /* FATFS 文件系统对象，用于挂载逻辑驱动器 */
UINT br, bw;                           /* 实际读取/写入的字节数 */
BYTE buffer[128];                      /* 用于读取文件内容的缓冲区 */
BYTE filebuffer[128];                  /* 用于存放通过串口写入文件的数据缓冲区 */

/************************ 函数声明 ************************/
ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length);
void nvic_config(void);
void write_file(void);

/************************************************************ 
 * Function :       System_Init
 * Comment  :       系统初始化，配置系统时钟
 * Parameter:       无
 * Return   :       无
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/
void System_Init(void)
{
    /* 配置SysTick定时器，为延时函数提供时基 */
    systick_config();
}

/************************************************************ 
 * Function :       UsrFunction
 * Comment  :       用户主功能函数，实现SD卡初始化、FATFS文件系统挂载、
 *                  文件创建/写入/读取验证等流程
 * Parameter:       无
 * Return   :       无
 * Author   :       Liu Tao @ GigaDevice
 * Date     :       2025-05-10 V0.1 original
************************************************************/
void UsrFunction(void)
{
    uint16_t k = 5;                  /* 重试次数计数器 */
    DSTATUS stat = 0;                /* 磁盘状态/操作结果 */
    nvic_config();                   /* 配置NVIC中断控制器，使能SDIO中断 */

    gd_eval_com_init();              /* 初始化串口，用于printf输出 */

    /* 带重试的SD卡磁盘初始化（物理驱动器0） */
    do
    {
        stat = disk_initialize(0);   /* 初始化SD卡，返回磁盘状态 */
    }while((stat != 0) && (--k));    /* 若失败且重试次数未耗尽则继续尝试 */

    printf("SD Card disk_initialize:%d\r\n",stat);

    /* 根据初始化结果和重试情况控制LED，并短暂停顿 */
    while(1){
        if(stat==0){
            // LED1_ON();           /* 初始化成功时可点亮LED1 */
        }
        if(k==0){
            LED2_ON();              /* 重试次数耗尽时点亮LED2，表示失败 */
        }
        if(stat!=0){
            LED3_ON();              /* 初始化失败时点亮LED3 */
        }
        delay_1ms(1000);
        break;                       /* 只执行一次循环体，立即跳出 */
    }
    
    /* 在物理驱动器0上挂载文件系统 */
    f_mount(0, &fs);
    printf("SD Card f_mount:%d\r\n",stat);

    /* 挂载成功后进行文件创建和读写测试 */
    if(RES_OK == stat)
    {        
        printf("\r\nSD Card Initialize Success!\r\n");
     
        /* 在SD卡根目录创建文件 FATFS.TXT（覆盖已有文件，允许写入） */
        result = f_open(&fdst, "0:/FATFS.TXT", FA_CREATE_ALWAYS | FA_WRITE);
     
        /* 调用串口输入函数，从用户获取要写入文件的内容，存入 filebuffer */
        write_file();

        /* 将 filebuffer 中的数据写入已打开的文件 */
        result = f_write(&fdst, filebuffer, sizeof(filebuffer), &bw);
        
        /* 检查写入结果 */
        if(FR_OK == result)
                printf("FATFS FILE write Success!\r\n");
        else
        {
                printf("FATFS FILE write failed!\r\n");
        }
        
        f_close(&fdst);               /* 关闭文件，确保数据写入磁盘 */

        /* 以只读方式重新打开文件，用于验证 */
        f_open(&fdst, "0:/FATFS.TXT", FA_OPEN_EXISTING | FA_READ);
        br = 1;                       /* 初始化实际读取字节数 */
        
        /* 循环读取文件内容到 buffer，直到读完或出错 */
        for(;;)
        {
            /* 清空读取缓冲区 */
            for (count=0; count<128; count++)
            {
                buffer[count]=0;
            }
            /* 从文件读取一块数据，br 返回实际读取字节数 */
            result = f_read(&fdst, buffer, sizeof(buffer), &br);
            if ((0 == result)|| (0 == br)) /* 读完或出错时退出 */
            {
                break;
            }
        }
        
        /* 比较读取到的内容与之前写入的 filebuffer 是否完全一致 */
        if(SUCCESS == memory_compare(buffer, filebuffer, 128))
        {
            printf("FATFS Read File Success!\r\nThe content is:%s\r\n",buffer);
        }
        else
        {
            printf("FATFS FILE read failed!\n");            
        }
        f_close(&fdst);              /* 关闭文件 */
    }
    
    /* 主循环，保持程序运行，周期性翻转PA5引脚（可用于LED闪烁指示） */
    while(1)
    {
        gpio_bit_set(GPIOA, GPIO_PIN_5); 
        delay_1ms(500);
        gpio_bit_reset(GPIOA, GPIO_PIN_5);  
        delay_1ms(500);
//      rtc_show_time();              /* 若需要可取消注释显示RTC时间 */
    }
}

/*!
    \brief      配置NVIC中断控制器，设置SDIO中断优先级并使能
    \param[in]  无
    \param[out] 无
    \retval     无
*/
void nvic_config(void)
{
    /* 设置中断优先级分组：1位抢占优先级，3位子优先级 */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
    /* 使能SDIO中断，抢占优先级0，子优先级0 */
    nvic_irq_enable(SDIO_IRQn, 0, 0);
}

/*!
    \brief      通过串口接收用户输入，并将其存入 filebuffer 中作为文件内容
    \param[in]  无
    \param[out] 无
    \retval     无
    \note       输入以回车('\r')结束，最多接收 sizeof(filebuffer)-1 个字符，
                并自动添加字符串结束符 '\0'
*/
void write_file(void)
{
    printf("Input data (press Enter to save):\r\n");
    
    uint16_t index = 0;    
    while(1){
        /* 如果USART0接收缓冲区非空 */
        if(usart_flag_get(USART0, USART_FLAG_RBNE) != RESET){
            char ch = usart_data_receive(USART0);   /* 从USART0读取一个字符 */
            if(ch == '\r'){                         /* 检测到回车键 */
                filebuffer[index] = '\0';           /* 添加字符串结束标志 */
                break;                              /* 结束接收 */
            }
            filebuffer[index++] = ch;               /* 存储接收到的字符并移动索引 */
            /* 防止缓冲区溢出：当达到缓冲区最大容量时强制退出 */
            if(index >= sizeof(filebuffer)-1) break;
        }
    }
}

/*!
    \brief      内存比较函数，逐字节比较两块内存区域是否相等
    \param[in]  src: 源数据指针
    \param[in]  dst: 目标数据指针
    \param[in]  length: 比较的数据长度（字节）
    \param[out] 无
    \retval     SUCCESS 表示完全匹配，ERROR 表示存在差异
*/
ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length) 
{
    while(length --){
        if(*src++ != *dst++)    /* 如果发现任何字节不匹配，立即返回错误 */
            return ERROR;
    }
    return SUCCESS;
}

/****************************End*****************************/

