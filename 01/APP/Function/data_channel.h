#ifndef __DATA_CHANNEL_H
#define __DATA_CHANNEL_H
#include "main.h"
float Channel_ReadCH0(void);   /* 电位器 × 变比 */
float Channel_ReadCH1(void);   /* DAC回读 × 变比 */
float Channel_ReadCH2(void);   /* PT100温度 */
void  Channel_AutoSample_SetInterval(uint8_t interval);  /* 预设间隔, 不立即开始 */
void  Channel_AutoSample_Start(uint8_t interval);        /* 1/3/5 秒 */
void  Channel_AutoSample_Stop(void);
void  Channel_AutoSample_Process(void);                   /* 主循环调用 */
#endif
