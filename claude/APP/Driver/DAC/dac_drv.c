#include "dac_drv.h"

static void timer5_config(void) {
    timer_parameter_struct timer_initpara;
    rcu_periph_clock_enable(RCU_TIMER5);
    timer_deinit(TIMER5);
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = 239;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 99;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER5, &timer_initpara);
    timer_master_output_trigger_source_select(TIMER5, TIMER_TRI_OUT_SRC_UPDATE);
    timer_enable(TIMER5);
}

void DAC_Init(void) {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_DAC);
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);
    dac_deinit(DAC0);
    dac_trigger_source_config(DAC0, DAC_OUT0, DAC_TRIGGER_T5_TRGO);
    dac_trigger_enable(DAC0, DAC_OUT0);
    dac_wave_mode_config(DAC0, DAC_OUT0, DAC_WAVE_DISABLE);
    dac_enable(DAC0, DAC_OUT0);
    timer5_config();
}

void DAC_SetValue(uint16_t val) {
    if (val > 4095) val = 4095;
    dac_trigger_disable(DAC0, DAC_OUT0);
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, val);
}

void DAC_SetVoltage(float v) {
    if (v < 0) v = 0;
    if (v > 3.3f) v = 3.3f;
    DAC_SetValue((uint16_t)(v * 4095.0f / 3.3f));
}
