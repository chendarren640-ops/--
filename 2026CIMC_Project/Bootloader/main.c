#include "bootloader.h"

int main(void) {
    systick_config();
    LED_Init();
    OLED_Init();
    OLED_ShowLine1((uint8_t*)"2026CIMC");
    OLED_ShowLine2((uint8_t*)"Bootloader");
    OLED_Refresh();

    /* 检查升级标记 */
    uint8_t *flag = (uint8_t*)0x08010000;
    if (flag[0] == 0xA5) {
        /* 升级模式: 打印倒计时提示, 等待10s */
        USART1_BL_Init();
        USART1_SendString("using command to interrupt start Application\r\n");
        for (int t=10; t>0; t--) {
            char buf[64]; sprintf(buf,"wait for start Application(%ds)......\r\n",t); USART1_SendString(buf);
            delay_1ms(1000);
            /* TODO: 检查是否收到0x0502命令 */
        }
        /* TODO: 固件接收+校验+搬运 */
    } else {
        /* 正常模式: 不输出, 延时5s, 跳转 */
        delay_1ms(5000);
    }
    JumpToApp(APP_ADDR);
    while(1);
}

void JumpToApp(uint32_t addr) {
    /* 检查APP入口是否有效 */
    if (*(volatile uint32_t*)addr == 0xFFFFFFFF) {
        /* APP不存在, 死循环 */
        while(1);
    }
    /* 重置所有外设 */
    rcu_periph_reset_enable();
    /* 设置MSP并跳转 */
    __set_MSP(*(volatile uint32_t*)addr);
    ((void(*)(void))(*(volatile uint32_t*)(addr+4)))();
}
