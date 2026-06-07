#ifndef __FLASH_PARAM_H
#define __FLASH_PARAM_H
#include "main.h"
#define PARAM_ADDR  0x08010000
#define PARAM_SIZE  4096

typedef struct {
    uint16_t device_id;
    uint8_t  baud_code;
    uint8_t  alarm_mode;
    float    ch0_ratio;
    float    ch1_ratio;
    float    ch0_threshold;
    float    ch1_threshold;
    float    ch2_threshold;   /* PT100 温度阈值 */
    uint8_t  upgrade_flag;
    uint8_t  reserved[3];
    uint32_t crc32;
} ParamBlock_t;

extern ParamBlock_t g_param;
void  Param_Load(void);
void  Param_Save(void);
void  Param_SetDefaults(void);
#endif
