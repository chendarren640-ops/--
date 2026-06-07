#include "adc_drv.h"
#define ADC_SAMPLES 8

void ADC_Init(void) {
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_ADC0);
    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0|GPIO_PIN_1);
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    adc_deinit();
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 2);
    adc_routine_channel_config(ADC0, 0, ADC_CHANNEL_10, ADC_SAMPLETIME_56);
    adc_routine_channel_config(ADC0, 1, ADC_CHANNEL_11, ADC_SAMPLETIME_56);
    adc_enable(ADC0);
    delay_1ms(1);
    adc_calibration_enable(ADC0);
}

/* 带超时的 EOC 等待, 防止死循环 */
static uint8_t adc_wait_eoc(void) {
    uint32_t timeout = 1000000;  /* ~100ms @ 240MHz */
    while (!adc_flag_get(ADC0, ADC_FLAG_EOC)) {
        if (--timeout == 0) return 0;
    }
    return 1;
}

float ADC_ReadCH0(void) {
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
    uint32_t sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        if (!adc_wait_eoc()) break;
        sum += adc_routine_data_read(ADC0);         /* rank 0 = CH10 */
        if (!adc_wait_eoc()) break;
        (void)adc_routine_data_read(ADC0);           /* 丢弃 rank 1 */
    }
    return (sum * 3.3f) / (ADC_SAMPLES * 4096.0f);
}

float ADC_ReadCH1(void) {
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
    uint32_t sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        if (!adc_wait_eoc()) break;
        (void)adc_routine_data_read(ADC0);           /* 丢弃 rank 0 */
        if (!adc_wait_eoc()) break;
        sum += adc_routine_data_read(ADC0);          /* rank 1 = CH11 */
    }
    return (sum * 3.3f) / (ADC_SAMPLES * 4096.0f);
}
