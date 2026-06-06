#include "flash_param.h"
ParamBlock_t g_param;

static uint32_t calc_crc32(uint8_t *data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) crc = (crc >> 1) ^ (crc & 1 ? 0xEDB88320 : 0);
    }
    return ~crc;
}

void Param_SetDefaults(void) {
    g_param.device_id      = 0x0001;
    g_param.baud_code      = 13;
    g_param.alarm_mode     = 0x02;
    g_param.ch0_ratio      = 1.0f;
    g_param.ch1_ratio      = 1.0f;
    g_param.ch0_threshold  = 100.0f;
    g_param.ch1_threshold  = 100.0f;
    g_param.upgrade_flag   = 0;
}

void Param_Load(void) {
    ParamBlock_t *flash_p = (ParamBlock_t *)PARAM_ADDR;
    uint32_t crc = calc_crc32((uint8_t*)flash_p, sizeof(ParamBlock_t) - 4);
    if (crc == flash_p->crc32) {
        memcpy(&g_param, (void*)flash_p, sizeof(ParamBlock_t));
    } else {
        Param_SetDefaults();
        Param_Save();
    }
}

void Param_Save(void) {
    fmc_unlock();
    fmc_page_erase(PARAM_ADDR);
    g_param.crc32 = calc_crc32((uint8_t*)&g_param, sizeof(ParamBlock_t) - 4);
    uint32_t *src = (uint32_t*)&g_param;
    for (int i = 0; i < sizeof(ParamBlock_t)/4; i++) {
        fmc_word_program(PARAM_ADDR + i*4, src[i]);
    }
    fmc_lock();
}
