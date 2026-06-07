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

//!开启之前一定要确保备份区有程序
#define Switch_App_Update_Fail_Test 0   //!测试升级失败的开关  0--关   1--开

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

//!备份App区程序
bool Backup_App(void);

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

	delay_1ms(100);


	printf("BootLoader : appVersion:0x%08x\r\n" , my_param_sum.BootParam.appVersion);
	printf("BootLoader : appVersion_Reserved:0x%08x\r\n" , my_param_sum.BootParam_Reserved.appVersion);

	if (my_param_sum.BootParam.appVersion != my_param_sum.BootParam_Reserved.appVersion)
	{
		//!进行App区的备份
		bool Backup_Result = Backup_App();
		if (Backup_Result == FALSE)
		{
			printf("BootLoader : Backup_App failed\r\n");
		}
		else
		{
			printf("BootLoader : Backup_App success\r\n");
		}
	}
	else
	{
		printf("BootLoader : appVersion is same\r\n");
	}


	my_param_sum.BootParam.appStartAddr = 0x800D000;
	my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);	//栈顶地址
	my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);	// 应用程序入口地址在应用程序起始地址后4字节(需要手动指定中断向量表的位置)


	//!测试升级失败 的开关  0--关   1--开
#if Switch_App_Update_Fail_Test

	my_param_sum.BootParam.updateStatus = 0x01;
	my_param_sum.BootParam.updateFlag = 0x5A;

	//设置一个错误的地址让其跳转失败
	my_param_sum.BootParam.appStartAddr = 0x810D000;
	my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);	//栈顶地址
	my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);	// 应用程序入口地址在应用程序起始地址后4字节(需要手动指定中断向量表的位置)
	printf("bootFailCount:%d\r\n" , my_param_sum.BootParam.bootFailCount);

#else	
	//A区程序的起始地址已经写死了，所以在B区程序的起始地址也写死了，如果需要修改A区程序的起始地址，需要修改这里的地址代码
	my_param_sum.BootParam.appStartAddr = 0x800D000;
	my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);	//栈顶地址
	my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);	// 应用程序入口地址在应用程序起始地址后4字节(需要手动指定中断向量表的位置)

#endif

	if (my_param_sum.BootParam.bootFailCount == 0xFFFF)
	{
		my_param_sum.BootParam.bootFailCount = 0;
	}

	if (my_param_sum.BootParam.bootFailCount >= 5)
	{
		printf("jump false , 版本回退\r\n");

		for (uint16_t i = 0; i < CONFIG_SIZE; i++)
		{
			config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
		}
		memcpy(&my_param_sum , config_buf , sizeof(Parameter_t));

		my_param_sum.BootParam.bootFailCount = 0;
		my_param_sum.BootParam.appVersion = my_param_sum.BootParam_Reserved.appVersion;

		//!擦除一页
		memcpy(config_buf , &my_param_sum , sizeof(Parameter_t));
		internal_flash_erase(BOOT_CONFIG_ADDR);
		internal_flash_write_str_Char(BOOT_CONFIG_ADDR , config_buf , CONFIG_SIZE);


		my_param_sum.BootParam.appStartAddr = 0x800D000;
		my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);	//栈顶地址
		my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);	// 应用程序入口地址在应用程序起始地址后4字节(需要手动指定中断向量表的位置)


		//将备份区内容搬运到App区
		//!回退版本
		for (uint8_t i = 0;i < 19; i++)
		{
			internal_flash_erase(0x800D000 + i * 4 * 1024);
		}
		internal_flash_write_str_Char(0x800D000 , (uint8_t*)0x8020000 , 76 * 1024);
	}


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

#if Switch_App_Update_Fail_Test
		Download_Transport_Result = FALSE;
#endif

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
	// printf("qqq\r\n");
	//!扇区4
	// internal_flash_earse_sector(0x8010000);
	// delay_1ms(500);
	// printf("aaa\r\n");
	//!扇区5
	// internal_flash_earse_sector(0x8020000);
	// delay_1ms(1000);
	// printf("eee\r\n");
	//!扇区6
	// internal_flash_earse_sector(0x8040000);
	// delay_1ms(1000);
	// printf("zzz\r\n");
	// //!将下载缓存区的应用程序参数搬运到App的地址
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

		for (uint16_t i = 0; i < CONFIG_SIZE; i++)
		{
			config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
		}
		memcpy(&my_param_sum , config_buf , sizeof(Parameter_t));

		my_param_sum.BootParam.bootFailCount++;
		printf("start false count : %d \r\n" , my_param_sum.BootParam.bootFailCount);
		memcpy(config_buf , &my_param_sum , sizeof(Parameter_t));
		internal_flash_erase(BOOT_CONFIG_ADDR);
		internal_flash_write_str_Char(BOOT_CONFIG_ADDR , config_buf , CONFIG_SIZE);

		mcu_software_reset();
		while (1);
	}

}

/************************************************************
 * Function :       Backup_App
 * Comment  :       用于备份应用程序
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2026-02-4 V0.1 original
************************************************************/
bool Backup_App(void)
{
	uint32_t crc32 = crc32_calc(((uint8_t*)0x800D000) , 76 * 1024);
	printf("BootLoader : backupCRC32:0x%08x , crc32:0x%08x\r\n" , my_param_sum.BootParam.backupCRC32 , crc32);
	if (crc32 != my_param_sum.BootParam.backupCRC32)
	{
		printf("BootLoader : Backup_App crc32 check fail\r\n");
		return FALSE;
	}

	for (uint8_t i = 0; i < 32; i++)
	{
		internal_flash_erase(0x8020000 + i * 1024 * 4);
		delay_1ms(30);
	}

	printf("BootLoader : Backup_App erase done\r\n");

	internal_flash_write_str_Char(0x8020000 , (uint8_t*)0x800D000 , 76 * 1024);

	uint32_t backupCRC32_1 = crc32_calc(((uint8_t*)0x8020000) , 76 * 1024);
	if (backupCRC32_1 != my_param_sum.BootParam.backupCRC32)
	{
		printf("ZZZZBootLoader : Backup_App crc32 check fail\r\n");
		return FALSE;
	}
	else
	{
		printf("ZZZZBootLoader : Backup_App crc32 check pass\r\n");
	}

	//!更新备份区的版本号
	for (uint16_t i = 0; i < CONFIG_SIZE; i++)
	{
		config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
	}
	memcpy(&my_param_sum , config_buf , sizeof(Parameter_t));
	//!更新版本号
	printf("BootLoader : Backup_App version:0x%08x -> 0x%08x \r\n" , my_param_sum.BootParam_Reserved.appVersion , my_param_sum.BootParam.appVersion);
	my_param_sum.BootParam_Reserved.appVersion = my_param_sum.BootParam.appVersion;
	memcpy(config_buf , &my_param_sum , sizeof(Parameter_t));
	//!写入
	internal_flash_erase(BOOT_CONFIG_ADDR);
	internal_flash_write_str_Char(BOOT_CONFIG_ADDR , config_buf , sizeof(Parameter_t));
	printf("BootLoader : Backup_App version : 0x%08x\r\n" , my_param_sum.BootParam_Reserved.appVersion);
	return TRUE;
}
/****************************End*****************************/
