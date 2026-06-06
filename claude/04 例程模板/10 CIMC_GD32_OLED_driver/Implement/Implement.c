/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：implement.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2023/2/16     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Implement.h"

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


__IO uint32_t prescaler_a = 0, prescaler_s = 0;
uint32_t RTCSRC_FLAG = 0;



/************************ 变量定义 ************************/

int adc_value;  // ADC采样值
float Vol_Value;  // ADC采样值转换成电压值


/************************ 函数定义 ************************/


///* 简单测试Flash读写 */
//void test_flash_read_write(void)
//{
//    uint8_t write_buffer[256];
//    uint8_t read_buffer[256];
//    uint32_t i, errors = 0;
//    
//    printf("\r\n开始Flash读写测试...\r\n");
//    
//    /* 准备测试数据 */
//    for(i = 0; i < 256; i++) {
//        write_buffer[i] = i;
//    }
//    
//    /* 擦除扇区0 */
//    printf("擦除扇区0...\r\n");
//    if(!flash_erase_sector_debug(0)) {
//        printf("扇区擦除失败!\r\n");
//        return;
//    }
//    
//    /* 写入测试数据 */
//    printf("写入测试数据...\r\n");
//    if(!flash_write_buffer(0, write_buffer, 256)) {
//        printf("数据写入失败!\r\n");
//        return;
//    }
//    
//    /* 读取数据进行验证 */
//    printf("读取数据进行验证...\r\n");
//    flash_read_data(0, read_buffer, 256);
//    
//    /* 验证数据 */
//    for(i = 0; i < 256; i++) {
//        if(write_buffer[i] != read_buffer[i]) {
//            printf("数据不匹配 - 地址: 0x%04X, 写入: 0x%02X, 读取: 0x%02X\r\n", 
//                   (unsigned int)i, write_buffer[i], read_buffer[i]);
//            errors++;
//            if(errors > 10) {
//                printf("错误过多，停止显示...\r\n");
//                break;
//            }
//        }
//    }
//    
//    if(errors) {
//        printf("测试失败 - 发现 %lu 处数据错误\r\n", errors);
//    } else {
//        printf("测试成功 - 所有数据匹配\r\n");
//    }
//}


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
	 rcu_periph_clock_enable(RCU_GPIOC);   // GPIOA时钟使能
	
	 gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0);   // GPIO 模式设置
	 rcu_periph_clock_enable(RCU_ADC0);    // 使能ADC时钟
	 adc_clock_config(ADC_ADCCK_PCLK2_DIV8);   // 配置时钟
	 ADC_Init();  // ADC配置
	 adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL); //  规则采样软件触发
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

	 unsigned char data[]=" ";
	 while(1)
	 {
		OLED_ShowString(0,0,"**Hello CIMC**",16);
		
        /* 延时1秒 */
    delay_1ms(1000);
		adc_flag_clear(ADC0,ADC_FLAG_EOC);  //  清除结束标志
		while(SET != adc_flag_get(ADC0,ADC_FLAG_EOC)){}  //  获取转换结束标志
        
    adc_value = ADC_RDATA(ADC0);  // 读取ADC数据
		Vol_Value = adc_value*3.3/4095;  //  转换成电压值
		printf("adc_value=%d\r\n",adc_value);         // 结果输出原始值
		printf("Vol_Value=%.2f\r\n",Vol_Value);         // 结果输出电压值
		sprintf(data,"Vol=%.2f V",Vol_Value);
			OLED_ShowString(0,16,data,16);
		 delay_1ms(1000);
		 OLED_Refresh();
	 }
}


/****************************End*****************************/

