/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：Function.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2026/2/4     V0.01    original
************************************************************/


#include "Function.h"
#include <time.h>
#include "rom.h"
#include "usart.h"
#include "bootloader.h"
#include "BootConfig.h"

/************************* 宏定义 *************************/

typedef uint8_t bool;
#define TRUE 1
#define FALSE 0

typedef void (*pFunction)(void);

pFunction jump2app;

#define CONFIG_SIZE 1024*4

#define CONFIG_APP_SIZE 1024*20

//!下载缓存区
#define APP_DOWNLOAD_ADDR 0x8073000	

/************************ 变量定义 ************************/

typedef struct __attribute__((packed)) Parameter_SUM
{
	BootParam_t BootParam;
	BootParam_t BootParam_Reserved;
	UpdateLog_t UpdateLog;
	UserConfig_t UserConfig;
	CalibData_t CalibData;
}Parameter_t;

Parameter_t my_param_sum = { 0 };

uint8_t config_buf[CONFIG_APP_SIZE] = { 0 };

/************************ 函数定义 ************************/

static void Analysis_ConfigForAddr(void);

static bool Download_Transport(uint32_t DownLoad_Addr);

uint32_t crc32_calc(uint8_t* data , uint32_t len);

//!软复位
void mcu_software_reset(void);

//!跳转程序
void jump_to_app(void);


/************************************************************
 * Function :       System_Init
 * Comment  :       用于初始化MCU
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-02-4 V0.1 original
************************************************************/

void System_Init(void)
{
	systick_config();

	my_usart_init();
}

/************************************************************
 * Function :       UsrFunction
 * Comment  :       用于用户功能实现
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-02-4 V0.1 original
************************************************************/
void UsrFunction(void)
{
	printf("BootLoader : start compare config\r\n");

	Analysis_ConfigForAddr();

	memcpy(&my_param_sum , config_buf , sizeof(Parameter_t));


	my_param_sum.BootParam.appStartAddr = 0x800D000;
	my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);	//栈顶地址
	my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);	// 应用程序入口地址在应用程序起始地址后4字节(需要手动指定中断向量表的位置)

	printf("BootLoader : appStackAddr:0x%08x\r\n" , my_param_sum.BootParam.appStackAddr);
	printf("BootLoader : appEntryAddr:0x%08x\r\n" , my_param_sum.BootParam.appEntryAddr);
	printf("BootLoader : appStartAddr:0x%08x\r\n" , my_param_sum.BootParam.appStartAddr);

	printf("BootLoader : appVersion:0x%08x\r\n" , my_param_sum.BootParam.appVersion);
	printf("BootLoader : updateStatus:0x%02x\r\n" , my_param_sum.BootParam.updateStatus);
	printf("BootLoader : updateFlag:0x%02x\r\n" , my_param_sum.BootParam.updateFlag);
	printf("BootLoader : magicWord:0x%08x\r\n" , my_param_sum.BootParam.magicWord);
	delay_1ms(1000);


	if (my_param_sum.BootParam.magicWord != 0x5AA5C33C)
	{
		printf("BootLoader : param magic is false\r\n");
		goto BootJump;
	}

	//!此时需要将搬运到备份地址的应用程序参数搬运到App的地址
	if (my_param_sum.BootParam.updateStatus == 0x01 && my_param_sum.BootParam.updateFlag == 0x5A)
	{
		printf("BootLoader : app is updating, need to copy param to app addr\r\n");

		bool Download_Transport_Result = Download_Transport(APP_DOWNLOAD_ADDR);


		for (uint16_t i = 0; i < CONFIG_SIZE; i++)
		{
			config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
		}
		memcpy(&my_param_sum , config_buf , sizeof(Parameter_t));

		if (Download_Transport_Result == TRUE)
		{
			my_param_sum.BootParam.appStartAddr = 0x800D000;
			my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);	//栈顶地址
			my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);	// 应用程序入口地址在应用程序起始地址后4字节(需要手动指定中断向量表的位置)
			my_param_sum.BootParam.updateStatus = 0x00;
			my_param_sum.BootParam.updateFlag = 0x00;
			my_param_sum.BootParam.updateCount++;
			// my_param_sum.BootParam.appVersion++;

			printf("BootLoader : app update success\r\n");
		}
		else
		{
			printf("BootLoader : app update fail\r\n");
			my_param_sum.BootParam.resetCount++;
			my_param_sum.BootParam.bootFailCount++;
		}
		//!擦除一页
		memcpy(config_buf , &my_param_sum , sizeof(Parameter_t));
		internal_flash_erase(BOOT_CONFIG_ADDR);
		internal_flash_write_str_Char(BOOT_CONFIG_ADDR , config_buf , CONFIG_SIZE);
		mcu_software_reset();
	}
	else
	{
	BootJump:
		jump_to_app();
	}



	while (1)
	{

	}
}
/************************************************************
 * Function :       Analysis_ConfigForAddr
 * Comment  :       用于分析配置参数中的地址
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-02-4 V0.1 original
************************************************************/
static void Analysis_ConfigForAddr(void)
{
	for (uint16_t i = 0; i < 1024 * 4; i++)
	{
		config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
	}
}
/************************************************************
 * Function :       Download_Transport
 * Comment  :       用于下载应用程序到指定地址
 * Parameter:       DownLoad_Addr 下载地址
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-02-4 V0.1 original
************************************************************/
static bool Download_Transport(uint32_t DownLoad_Addr)
{
	printf("BootLoader : appSize:%d\r\n" , my_param_sum.BootParam.appSize);
	if (my_param_sum.BootParam.appSize == 0)
	{
		return FALSE;
	}
	//!App区的总大小为 3 * 4 + 64 + 128 + 128 = 332KB
	//!先擦除3页(3 * 4 = 12kb)
	for (uint8_t i = 0; i < 3; i++)
	{
		internal_flash_erase(my_param_sum.BootParam.appStartAddr + i * 4 * 1024);
	}

	memset(config_buf , 0 , sizeof(config_buf));

	for (uint16_t i = 0; i < my_param_sum.BootParam.appSize; i++)
	{
		config_buf[i] = internal_flash_read_Char(APP_DOWNLOAD_ADDR + i);
	}
	internal_flash_write_str_Char(my_param_sum.BootParam.appStartAddr , config_buf , my_param_sum.BootParam.appSize);

	uint32_t app_crc32 = crc32_calc(config_buf , my_param_sum.BootParam.appSize);

	printf("BootLoader : appCRC32:0x%08x , app_crc32:0x%08x\r\n" , my_param_sum.BootParam.appCRC32 , app_crc32);
	if (app_crc32 == my_param_sum.BootParam.appCRC32)
	{
		printf("app crc32 check pass\r\n");
		return TRUE;
	}
	else
	{
		printf("app crc32 check fail\r\n");
		return FALSE;
	}
}
/************************************************************
 * Function :       crc32_calc
 * Comment  :       用于计算CRC32校验值
 * Parameter:       data 数据指针
 * Parameter:       len 数据长度
 * Return   :       CRC32校验值
 * Author   :       Jialei Zhao
 * Date     :       2026-02-4 V0.1 original
************************************************************/
uint32_t crc32_calc(uint8_t* data , uint32_t len)
{
	uint32_t crc = 0xFFFFFFFF;
	uint32_t i , j;

	for (i = 0; i < len; i++)
	{
		crc ^= data[i];
		for (j = 0; j < 8; j++)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xEDB88320;
			else
				crc >>= 1;
		}
	}

	return crc ^ 0xFFFFFFFF;
}
/************************************************************
 * Function :       mcu_software_reset
 * Comment  :       用于软件复位
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-02-4 V0.1 original
************************************************************/
void mcu_software_reset(void)
{
	/* set FAULTMASK */
	__set_FAULTMASK(1);
	NVIC_SystemReset();
}
/************************************************************
 * Function :       iap_load_app
 * Comment  :       用于加载应用程序到指定地址
 * Parameter:       appxaddr 应用程序地址
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-02-4 V0.1 original
************************************************************/
void iap_load_app(uint32_t appxaddr)
{
	if (((*(__IO uint32_t*)appxaddr) & 0x2FFE0000) == 0x20000000)//check if it is legal
	{
		/* 关闭所有中断 */
		__disable_irq();

		/* 关闭 SysTick */
		SysTick->CTRL = 0;
		SysTick->LOAD = 0;
		SysTick->VAL = 0;

		/* 清除所有 NVIC 中断使能和挂起位 */
		for (uint32_t i = 0; i < 8; i++)
		{
			NVIC->ICER[i] = 0xFFFFFFFF;
			NVIC->ICPR[i] = 0xFFFFFFFF;
		}

		/* 等待所有中断清除完成 */
		__DSB();
		__ISB();

		/* 设置新的中断向量表 */
		SCB->VTOR = appxaddr;

		/* 设置新的栈指针 */
		__set_MSP(*(__IO uint32_t*)appxaddr);

		/* 跳转到应用程序 */
		jump2app = (pFunction)(*(__IO uint32_t*)(appxaddr + 4));
		jump2app();

		/* 如果执行到这里说明跳转失败 */
		printf("jump to app fail\r\n");
		while (1);
	}
}

/************************************************************
 * Function :       jump_to_app
 * Comment  :       用于跳转到应用程序
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-02-4 V0.1 original
************************************************************/
void jump_to_app(void)
{
	// It's better to disable IRQs before jump
	if ((my_param_sum.BootParam.appEntryAddr & 0xFF000000) == 0x8000000)		//judge if the app code is legal
	{
		iap_load_app(my_param_sum.BootParam.appStartAddr);//run FLASH APP
	}
	else
	{
		// send_ack(NO_STD_FIRMWARE);
		printf("zzz\r\n");
		while (1);
		// mcu_software_reset();
		// while (1);
	}

}

/****************************End*****************************/
