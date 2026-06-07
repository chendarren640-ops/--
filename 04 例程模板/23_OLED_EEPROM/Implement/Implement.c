/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：implement.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2023/2/16     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Implement.h"
#include "eeprom.h"

/************************* 宏定义 *************************/

uint32_t int_device_serial[3];
uint8_t count;
__IO uint32_t TimingDelay = 0;

uint8_t tx_buffer[256];
uint8_t rx_buffer[256];
uint32_t flash_id = 0;
uint32_t DeviceID = 0;
uint16_t i = 0;
uint8_t  is_successful = 0;

rtc_parameter_struct   rtc_initpara;


__IO uint32_t prescaler_a = 0 , prescaler_s = 0;
uint32_t RTCSRC_FLAG = 0;



/************************ 变量定义 ************************/


/************************ 函数定义 ************************/


/************************************************************
 * 函数:       System_Init(void)
 * 说明:       系统初始化
 * 输入:       无
 * 输出:       无
 * 返回值:     无
 * 作者        Lingyu Meng
 * 其他:       无
************************************************************/

void System_Init(void)
{
	systick_config();     // 时钟配置
	USART0_Config();     // 串口初始化
	LED_Init();          // LED 初始化
	OLED_Init();
	delay_1ms(10);
}

/************************************************************
 * 函数:       function(void)
 * 说明:       执行函数
 * 输入:       无
 * 输出:       无
 * 返回值:     无
 * 作者        Lingyu Meng
 * 其他:       无
************************************************************/

void UsrFunction(void)
{

	uint8_t data[16] = { 0 };
	float adc_value11 = 0;

	uint8_t* str1 = (uint8_t*)"Hello EEPROM";
	//空数组	用存放从eeprom中读取到的数据
	uint8_t data1[13] = { 0 };


	while (1)
	{

		//!====EEPROM测试=====

		EEPROM_WriteStr(0x00 , str1 , 12);
		printf("Write: %s \r\n" , str1);

		EEPROM_ReadStr(0x00 , data1 , 12);
		printf("Read: %s \r\n" , data1);

		for (int i = 0; i < 12; i++)
		{
			printf("%02x " , data1[i]);
		}
		printf("\r\n");

		if (memcmp(data1 , str1 , 12) == 0)
		{
			// printf("data: %s\r\n" , data);
			printf("EEPROM Test Passed! ---- Test success\r\n");
		}
		else
		{
			// printf("data: %s\r\n" , data);
			printf("EEPROM Test Failed!\r\n");
		}

		//!====OLED显示测试=====


		OLED_ShowString(0 , 0 , "**Hello CIMC**" , 16);

		adc_value11 += 1.23f;
		OLED_ShowString(0 , 16 , data , 16);
		sprintf((char*)data , "V = %.2fV" , adc_value11);

		delay_1ms(1000);
		OLED_Refresh();
	}
}


/****************************End*****************************/

