
/************************* 头文件 *************************/

#include "sysFunction.h"
#include "LED.h"        // LED控制头文件
//#include "RTC.h"        // 实时时钟头文件
//#include "USART.h"      // 串口通信头文件
#include "SPI_FLASH.h"  // SPI Flash操作头文件


/************************* 宏定义 *************************/

uint32_t int_device_serial[3]; // 设备序列号存储
uint8_t count;                 // 通用计数器
__IO uint32_t TimingDelay = 0; // 延时计数器(volatile修饰)


#define  SFLASH_ID                     0xC84013   // Flash芯片的预期ID值
#define  TEAM_ID                        2025239771  //Flash芯片的预期队伍编号值
#define  BUFFER_SIZE                    256        // 缓冲区大小
#define  TX_BUFFER_SIZE                   256        // 缓冲区大小
#define  FLASH_WRITE_ADDRESS           0x000000   // Flash写入起始地址
#define  FLASH_READ_ADDRESS            FLASH_WRITE_ADDRESS // Flash读取起始地址（与写入地址相同）

uint32_t DeviceID = 0;
uint32_t flash_id = 0;          // 存储读取到的Flash芯片ID
uint32_t team_id =2025975022;   // 存储读取到的Flash芯片队伍编号
uint8_t  tx_buffer[TX_BUFFER_SIZE]; // 发送缓冲区（用于写入Flash的数据）
uint8_t  rx_buffer[TX_BUFFER_SIZE]; // 接收缓冲区（用于从Flash读取的数据）
uint16_t i = 0;                 // 通用循环计数器
uint8_t  is_successful = 0;     // 操作成功标志（0=成功，1=失败）



/************************ 变量定义 ************************/

int adc_value;       // ADC采样值

float Vol_Value;     // ADC采样值转换后的电压值


/************************ 函数定义 ************************/

ErrStatus memory_compare(uint8_t* src,uint8_t* dst,uint16_t length);   //内存比较函数声明

/************************************************************ 
 * Function :       System_Init
 * Comment  :       用于初始化MCU
************************************************************/

void System_Init(void)
{
	systick_config();     // 初始化SysTick定时器，用于延时函数
	
	// 先初始化串口，确保后续printf可用
    USART0_Config();     // 串口初始化
	
	 delay_1ms(10); // 10ms延时确保电源稳定
	
	// 打印初始化开始信息（此时串口应已可用）
    printf("\n\r====system init====\n\r");

  rcu_periph_clock_enable(RCU_GPIOC);   // 使能GPIOC时钟

// 设置GPIOC的0号引脚为模拟输入模式（用于ADC）

  gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0);

  rcu_periph_clock_enable(RCU_ADC0);    // 使能ADC0时钟

  adc_clock_config(ADC_ADCCK_PCLK2_DIV8);   // 配置ADC时钟为PCLK2的8分频

  ADC_Init();  // 初始化ADC（此函数需在Implement.h中声明）

  adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL); // 使能规则通道的软件触发
	
	
	LED_Init();          // 初始化LED

  OLED_Init();         // 初始化OLED

  //flash_init();        //初始化flash
	
  //OLED_Clear();        //OLED清屏
	
  delay_1ms(100);       // 延时10ms，等待外设稳定
	
	nvic_irq_enable(USART0_IRQn, 0, 0);//使能USART0中断
	
	usart_interrupt_enable(USART0, USART_INT_RBNE);//接收中断打开
	
}


/************************************************************ 
 * Function :       UsrFunction
 * Comment  :       用户程序功能:外部Flash读写

************************************************************/


uint8_t string[30]={"this is flash test"};// 测试用的字符串数据



void UsrFunction(void)
{
	
    delay_1ms(50);// 确保系统已稳定
	
	
	//printf("====system init====\r\n");  //串口打印文字
	
	// 通过串口发送系统启动信息
   // printf("\n\r###############################################################################\n\r");
   // printf("\n\rGD32470 System is Starting up...\n\r");
   // printf("\n\rGD32470 SystemCoreClock:%dHz\n\r", SystemCoreClock);
	
	  unsigned char data[]=" "; // 显示缓冲区
	
    //gd_eval_com_init();   // 初始化串口（用于调试输出）
    
    /* 配置SPI0 GPIO和相关参数 */
    spi_flash_init();     // 初始化SPI Flash控制器
    
    /* 获取Flash芯片ID */
    flash_id = spi_flash_read_id(); // 读取Flash芯片的ID号
    printf("\n\rDevice_ID:0x%X\n\r",flash_id); // 打印读取到的ID值
	  printf("\n\r2025-CIMC-队伍编号：%lu\n\r", team_id); // 打印读取到的队伍编号值
	  printf("====system ready====\r\n");  //串口打印文字
		
		 OLED_ShowString(0,0,"system idle",16);
    OLED_Refresh();
		
	while(1) // 主循环
    {
        // OLED显示信息
       // OLED_ShowString(0,0,"system idle",16);
        
        // 延时1秒
        delay_1ms(1000);
		}
	
	}
	
	
	/*
    //验证Flash ID是否正确 
    if(SFLASH_ID == flash_id) // 如果ID匹配
    {
        // 擦除Flash扇区
        printf("\n\r\n\r******************************erases flash sector*************************\n\r\n\r");
        spi_flash_sector_erase(FLASH_WRITE_ADDRESS); // 擦除指定地址所在的扇区
        
        // 准备测试数据并打印
        printf("\n\r\n\r******************************Write to tx_buffer:*************************\n\r\n\r");
        for(i = 0; i < BUFFER_SIZE; i ++){
            tx_buffer[i] = i;          // 填充发送缓冲区（0-255）
            printf("0x%02X ",tx_buffer[i]); // 以16进制格式打印数据
            
            if(15 == i%16)             // 每16个数据换行
                printf("\n\r");
						
						
     }
	

        spi_flash_buffer_write(tx_buffer, FLASH_WRITE_ADDRESS, TX_BUFFER_SIZE);//将数据写入Flash
        
        delay_1ms(10); // 等待写入完成（短延时）

        // 从Flash读取数据
        printf("\n\r\n\r******************************Read from tx_buffer:*************************\n\r\n\r");
        spi_flash_buffer_read(rx_buffer, FLASH_READ_ADDRESS, RX_BUFFER_SIZE);
        
        // 打印读取到的数据
        for(i = 0; i <= 255; i ++){
            printf("0x%02X ", rx_buffer[i]);
            if(15 == i%16)
            printf("\n\r");
        }
        
        // 比较写入和读出的数据是否一致 
        if(ERROR == memory_compare(tx_buffer, rx_buffer, 256)){
            printf("Err:Data Read and Write aren't Matching.\n\r");
            is_successful = 1; // 标记数据不匹配
        }
        
        //根据测试结果输出信息 
        if(0 == is_successful){
            gpio_bit_set(GPIOA, GPIO_PIN_6); // 测试通过，点亮LED（PA6）
            printf("\n\rSPI-GD25Q40ESIGR Test Passed!\n\r");
        } else {
           
            printf("\n\rSPI Flash: Read ID Fail!\n\r");//Flash ID验证失败
        }
        
        // 字符串读写测试 
        spi_flash_buffer_erase(0, 18);        // 从0地址擦除18字节
        spi_flash_buffer_write(string, 0, 18); // 写入字符串
        spi_flash_buffer_read(rx_buffer, 0, 18); // 读取字符串
        
        // 打印读取的字符串
        printf("String variable reading\r\n");
        for(i = 0; i < 18; i ++){
            printf("%c", rx_buffer[i]); // 以字符格式输出
        }
    }
    
    // 主循环（LED闪烁）
    while(1)
    {
        gpio_bit_set(GPIOA, GPIO_PIN_5);   // 点亮LED（PA5）
        delay_1ms(500);                    // 延时500ms
        gpio_bit_reset(GPIOA, GPIO_PIN_5);  // 熄灭LED
        delay_1ms(500);                    // 延时500ms
    }
}


*/
/*
    \brief      内存比较函数
    \param[in]  src: 源数据指针
    \param[in]  dst: 目标数据指针
    \param[in]  length: 需要比较的数据长度
    \param[out] none
    \retval     ErrStatus: 返回ERROR(不匹配)或SUCCESS(匹配)
*/

/*
ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length) 
{
    while(length--){  // 遍历每个字节
        if(*src++ != *dst++) // 比较当前字节
            return ERROR;    // 发现不匹配立即返回错误
    }
    return SUCCESS; // 全部匹配返回成功
}
	
	*/
	
	
	
	
	
	


/****************************End*****************************/

