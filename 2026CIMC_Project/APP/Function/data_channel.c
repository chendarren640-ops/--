#include "data_channel.h"
#include "adc_drv.h"
#include "flash_param.h"
float Channel_ReadCH0(void) { return ADC_ReadCH0() * g_param.ch0_ratio; }
float Channel_ReadCH1(void) { return ADC_ReadCH1() * g_param.ch1_ratio; }
float Channel_ReadCH2(void) { return 25.0f; } /* TODO: PT100 */
void  Channel_AutoSample_Start(uint8_t interval) {}
void  Channel_AutoSample_Stop(void) {}
