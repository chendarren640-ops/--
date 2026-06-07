/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：RTC.c
 * 作者: Qiao Qin @ GigaDevice
 * 平台: 2025CIMC IHD-V04
 * 版本: Qiao Qin     2025/4/20     V0.01    original
 ************************************************************/

#include "RTC.h"
#include "LED.h"

/* 选择RTC的时钟源，使用外部低速晶振(LXTAL) */
#define RTC_CLOCK_SOURCE_LXTAL
// #define RTC_CLOCK_SOURCE_IRC32K   /* 若使用内部32K RC振荡器则取消注释本行 */

/* 备份寄存器标志值，用于判断RTC是否已经配置过 */
#define BKP_VALUE 0x32F0

/* 全局变量定义 */
rtc_parameter_struct rtc_initpara;   // RTC时间参数结构体
rtc_alarm_struct rtc_alarm;          // RTC闹钟参数结构体
__IO uint32_t prescaler_a = 0, prescaler_s = 0; // 异步/同步预分频值
uint32_t RTCSRC_FLAG = 0;            // RTC时钟源标志

/*!
    \brief      RTC初始化主函数
    \param[in]  无
    \param[out] 无
    \retval     无
*/
void RTC_Init(void)
{
    /* 打印启动提示信息 */
    printf("\n\r  ****************** RTC calendar demo ******************\n\r");

    /* 使能电源管理单元(PMU)时钟 */
    rcu_periph_clock_enable(RCU_PMU);
    /* 允许对RTC备份域寄存器进行写操作 */
    pmu_backup_write_enable();

    /* 进行RTC时钟源等预配置 */
    rtc_pre_config();
    /* 读取备份域控制寄存器(RCU_BDCTL)中RTC时钟源选择位(位8~9)，
       这里取反后作为标志：若选择位全0则表示未配置，RTCSRC_FLAG = 1；
       否则RTCSRC_FLAG = 0 */
    RTCSRC_FLAG = !GET_BITS(RCU_BDCTL, 8, 9);

    /* 检查备份寄存器BKP0的值是否等于预设标志，以及RTC时钟源是否已配置 */
    if ((BKP_VALUE != RTC_BKP0) || (0x00 == RTCSRC_FLAG))
    {
        /* 条件成立说明：
           1. 备份寄存器值不正确（首次上电或VBAT掉电导致备份域数据丢失）
           2. 或者RTC时钟源尚未配置
           此时需要进行RTC时间与闹钟的设置流程 */
        rtc_setup();
    }
    else
    {
        /* RTC已经配置过，仅检测本次复位原因并显示当前时间 */
        if (RESET != rcu_flag_get(RCU_FLAG_PORRST))
        {
            printf("power on reset occurred....\n\r");   // 上电复位
        }
        else if (RESET != rcu_flag_get(RCU_FLAG_EPRST))
        {
            printf("external reset occurred....\n\r");   // 外部复位
        }
        printf("no need to configure RTC....\n\r");

        /* 直接从RTC读取并显示当前时间 */
        rtc_show_time();
    }
    /* 清除所有复位标志，避免影响后续判断 */
    rcu_all_reset_flag_clear();
}

/*!
    \brief      RTC预配置函数：选择时钟源、计算预分频值并使能RTC时钟
    \param[in]  无
    \param[out] 无
    \retval     无
*/
void rtc_pre_config(void)
{
#if defined(RTC_CLOCK_SOURCE_IRC32K)
    /* 使用内部32KHz低速RC振荡器 */
    rcu_osci_on(RCU_IRC32K);              // 开启IRC32K
    rcu_osci_stab_wait(RCU_IRC32K);       // 等待振荡器稳定
    rcu_rtc_clock_config(RCU_RTCSRC_IRC32K); // 选择RTC时钟源为IRC32K

    prescaler_s = 0x13F;   // 同步预分频值 (319) 产生约100Hz的ck_spre
    prescaler_a = 0x63;    // 异步预分频值 (99)  32KHz/(99+1)=320Hz,
                           // 再经过同步分频(319+1) → 1Hz时钟
#elif defined(RTC_CLOCK_SOURCE_LXTAL)
    /* 使用外部32.768KHz晶振 */
    rcu_osci_on(RCU_LXTAL);              // 开启LXTAL
    rcu_osci_stab_wait(RCU_LXTAL);       // 等待晶振稳定
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL); // 选择RTC时钟源为LXTAL

    prescaler_s = 0xFF;    // 同步预分频值 (255)
    prescaler_a = 0x7F;    // 异步预分频值 (127)  32.768KHz/(127+1)=256Hz,
                           // 再经过同步分频(255+1) → 1Hz时钟
#else
#error RTC clock source should be defined.
#endif /* RTC_CLOCK_SOURCE_IRC32K */

    /* 使能RTC外设时钟，并等待寄存器同步完成 */
    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();
}

/*!
    \brief      通过串口交互设置RTC时间（和闹钟，闹钟部分已注释）
    \param[in]  无
    \param[out] 无
    \retval     无
*/
void rtc_setup(void)
{
    /* 存放用户输入的年、月、日、时、分、秒，初始化为无效值0xFF */
    uint32_t tmp_year = 0xFF, tmp_month = 0xFF, tmp_day = 0xFF;
    uint32_t tmp_hh = 0xFF, tmp_mm = 0xFF, tmp_ss = 0xFF;

    /* 配置RTC初始化结构体的基本参数 */
    rtc_initpara.factor_asyn = prescaler_a;   // 异步预分频因子
    rtc_initpara.factor_syn  = prescaler_s;   // 同步预分频因子
    rtc_initpara.year        = 0x16;          // 年份预设为16（即2016年，仅低两位）
    rtc_initpara.day_of_week = RTC_SATURDAY;  // 星期预设为星期六
    rtc_initpara.month       = RTC_APR;       // 月份预设为4月
    rtc_initpara.date        = 0x30;          // 日期预设为30号
    rtc_initpara.display_format = RTC_24HOUR; // 24小时制
    rtc_initpara.am_pm       = RTC_AM;        // 上午（在24小时制下无效）

    /* 以下通过串口逐个获取年、月、日、时、分、秒的有效输入 */
    printf("=======Configure RTC Time========\n\r");
    printf("  please set the last two digits of current year:\n\r");
    while (tmp_year == 0xFF)
    {
        /* usart_input_threshold(99) 要求输入0~99的数字，
           返回BCD格式的值，若非法则返回0xFF，循环直至有效 */
        tmp_year = usart_input_threshold(99);
        rtc_initpara.year = tmp_year;
    }
    printf("  20%0.2x\n\r", tmp_year);

    printf("  please input month:\n\r");
    while (tmp_month == 0xFF)
    {
        tmp_month = usart_input_threshold(12);
        rtc_initpara.month = tmp_month;
    }
    printf("  %0.2x\n\r", tmp_month);

    printf("  please input day:\n\r");
    while (tmp_day == 0xFF)
    {
        tmp_day = usart_input_threshold(31);
        rtc_initpara.date = tmp_day;
    }
    printf("  %0.2x\n\r", tmp_day);

    printf("  please input hour:\n\r");
    while (0xFF == tmp_hh)
    {
        tmp_hh = usart_input_threshold(23);
        rtc_initpara.hour = tmp_hh;
    }
    printf("  %0.2x\n\r", tmp_hh);

    printf("  please input minute:\n\r");
    while (0xFF == tmp_mm)
    {
        tmp_mm = usart_input_threshold(59);
        rtc_initpara.minute = tmp_mm;
    }
    printf("  %0.2x\n\r", tmp_mm);

    printf("  please input second:\n\r");
    while (0xFF == tmp_ss)
    {
        tmp_ss = usart_input_threshold(59);
        rtc_initpara.second = tmp_ss;
    }
    printf("  %0.2x\n\r", tmp_ss);

    /* 调用库函数 rtc_init 将设置写入RTC寄存器 */
    if (ERROR == rtc_init(&rtc_initpara))
    {
        /* 配置失败：点亮LED1报警 */
        LED1_ON();
    }
    else
    {
        /* 配置成功：打印成功信息，显示时间，将标志写入备份寄存器，并点亮LED2~4 */
        printf("\n\r** RTC time configuration success! **\n\r");
        rtc_show_time();
        RTC_BKP0 = BKP_VALUE;   // 将预设值写入备份寄存器，标记已配置
        LED2_ON();
        LED3_ON();
        LED4_ON();
    }

    /* 闹钟配置部分已被注释，若需使用可取消注释 */
    //    tmp_hh = 0xFF;
    //    tmp_mm = 0xFF;
    //    tmp_ss = 0xFF;
    //
    //    rtc_alarm_disable(RTC_ALARM0);
    //    printf("=======Input Alarm Value=======\n\r");
    //    rtc_alarm.alarm_mask = RTC_ALARM_DATE_MASK|RTC_ALARM_HOUR_MASK|RTC_ALARM_MINUTE_MASK;
    //    rtc_alarm.weekday_or_date = RTC_ALARM_DATE_SELECTED;
    //    rtc_alarm.alarm_day = 0x31;
    //    rtc_alarm.am_pm = RTC_AM;
    //
    //    printf("  please input Alarm Hour:\n\r");
    //    while (0xFF == tmp_hh){
    //        tmp_hh = usart_input_threshold(23);
    //        rtc_alarm.alarm_hour = tmp_hh;
    //    }
    //    printf("  %0.2x\n\r", tmp_hh);
    //
    //    printf("  Please Input Alarm Minute:\n\r");
    //    while (0xFF == tmp_mm){
    //        tmp_mm = usart_input_threshold(59);
    //        rtc_alarm.alarm_minute = tmp_mm;
    //    }
    //    printf("  %0.2x\n\r", tmp_mm);
    //
    //    printf("  Please Input Alarm Second:\n\r");
    //    while (0xFF == tmp_ss){
    //        tmp_ss = usart_input_threshold(59);
    //        rtc_alarm.alarm_second = tmp_ss;
    //    }
    //    printf("  %0.2x", tmp_ss);
    //
    //    rtc_alarm_config(RTC_ALARM0,&rtc_alarm);
    //    printf("\n\r** RTC Set Alarm Success!  **\n\r");
    //    rtc_show_alarm();
    //
    //    rtc_interrupt_enable(RTC_INT_ALARM0);
    //    rtc_alarm_enable(RTC_ALARM0);
}

/*!
    \brief      从RTC读取当前时间并通过串口显示
    \param[in]  无
    \param[out] 无
    \retval     无
*/
void rtc_show_time(void)
{
    /* 读取当前完整时间到 rtc_initpara 结构体中 */
    rtc_current_time_get(&rtc_initpara);

    /* 以下为获取亚秒值的代码（已注释），若需更高精度可取消注释 */
    //    uint32_t time_subsecond = 0;
    //    uint8_t subsecond_ss = 0,subsecond_ts = 0,subsecond_hs = 0;
    //    time_subsecond = rtc_subsecond_get();
    //    subsecond_ss=(1000-(time_subsecond*1000+1000)/400)/100;
    //    subsecond_ts=(1000-(time_subsecond*1000+1000)/400)%100/10;
    //    subsecond_hs=(1000-(time_subsecond*1000+1000)/400)%10;

    /* 打印当前日期和时间（年份是20xx，月、日、时、分、秒均为BCD格式） */
    printf("\r\nCurrent time: 20%0.2x-%0.2x-%0.2x",
           rtc_initpara.year, rtc_initpara.month, rtc_initpara.date);
    printf(" : %0.2x:%0.2x:%0.2x \r\n",
           rtc_initpara.hour, rtc_initpara.minute, rtc_initpara.second);
}

/*!
    \brief      读取并显示闹钟0的设定值（当前函数体保留，但主流程中闹钟配置被注释）
    \param[in]  无
    \param[out] 无
    \retval     无
*/
void rtc_show_alarm(void)
{
    /* 获取闹钟0的设定值到 rtc_alarm 结构体 */
    rtc_alarm_get(RTC_ALARM0, &rtc_alarm);
    printf("The alarm: %0.2x:%0.2x:%0.2x \n\r",
           rtc_alarm.alarm_hour, rtc_alarm.alarm_minute, rtc_alarm.alarm_second);
}

/*!
    \brief      从串口接收两个ASCII数字字符，转换为BCD格式，并进行范围检查
    \param[in]  value : 允许的最大数值（十进制）
    \param[out] 无
    \retval     成功返回BCD格式的数值（高4位为十位，低4位为个位），
                失败（输入非法或超范围）返回0xFF
*/
uint8_t usart_input_threshold(uint32_t value)
{
    uint32_t index = 0;
    uint32_t tmp[2] = {0, 0};  // 暂存两个ASCII字符

    /* 循环接收两个字符 */
    while (index < 2)
    {
        /* 等待USART0接收缓冲区非空 */
        while (RESET == usart_flag_get(USART0, USART_FLAG_RBNE))
            ;
        /* 读取一个字节并存入tmp数组 */
        tmp[index++] = usart_data_receive(USART0);
        /* 检查是否为数字字符 '0'~'9' (ASCII 0x30~0x39) */
        if ((tmp[index - 1] < 0x30) || (tmp[index - 1] > 0x39))
        {
            printf("\n\r please input a valid number between 0 and 9 \n\r");
            index--;  // 非数字则回退索引，要求重新输入
        }
    }

    /* 将两个字符转换为十进制数值：十位 * 10 + 个位 */
    index = (tmp[1] - 0x30) + ((tmp[0] - 0x30) * 10);
    if (index > value)  // 检查数值是否在允许范围内
    {
        printf("\n\r please input a valid number between 0 and %d \n\r", value);
        return 0xFF;   // 超出范围返回错误标志
    }

    /* 将十进制转换为BCD码：高4位放十位，低4位放个位 */
    /* 注意：此处重新计算，前面 index 只是临时存放十进制数，现覆盖为BCD值 */
    index = (tmp[1] - 0x30) + ((tmp[0] - 0x30) << 4);
    return (uint8_t)index;
}

