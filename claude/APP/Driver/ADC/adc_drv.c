/**
 * ADC 驱动 — 单次转换模式
 * CH0: PC0 ADC0_CH10 (电位器)
 * CH1: PC1 ADC0_CH11 (DAC回读)
 */
#include "adc_drv.h"

void ADC_Init(void) {
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_ADC0);

    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1);

    adc_deinit();
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, DISABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_resolution_config(ADC0, ADC_RESOLUTION_12B);

    /* 单次转换, 不扫描 — 每次读重新配置通道 */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);

    adc_enable(ADC0);
    delay_1ms(1);
    adc_calibration_enable(ADC0);
}

/* 轮询方式读指定通道, 带超时 */
static uint16_t adc_read_ch(uint8_t channel) {
    uint32_t timeout;

    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1);
    adc_routine_channel_config(ADC0, 0, channel, ADC_SAMPLETIME_480);
    adc_flag_clear(ADC0, ADC_FLAG_EOC);
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);

    timeout = 100000;
    while (!adc_flag_get(ADC0, ADC_FLAG_EOC) && timeout > 0) timeout--;
    if (timeout == 0) return 0;

    return (uint16_t)(ADC_RDATA(ADC0) & 0xFFFF);
}

float ADC_ReadCH0(void) {
    return (adc_read_ch(ADC_CHANNEL_10) * 3.3f) / 4095.0f;
}

float ADC_ReadCH1(void) {
    return (adc_read_ch(ADC_CHANNEL_11) * 3.3f) / 4095.0f;
}
