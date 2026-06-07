#include "dac_drv.h"

void DAC_Init(void) {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_DAC);
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);
    dac_deinit(DAC0);
    dac_trigger_source_config(DAC0, DAC_OUT0, DAC_TRIGGER_SOFTWARE);
    dac_trigger_enable(DAC0, DAC_OUT0);
    dac_enable(DAC0, DAC_OUT0);
}

void DAC_SetValue(uint16_t val) {
    if(val>4095)val=4095;
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, val);
    dac_software_trigger_enable(DAC0, DAC_OUT0);
}

void DAC_SetVoltage(float v) {
    if(v<0)v=0;if(v>3.3f)v=3.3f;
    DAC_SetValue((uint16_t)(v*4095.0f/3.3f));
}
