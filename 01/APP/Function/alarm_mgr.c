#include "alarm_mgr.h"
#include "rtc_drv.h"
#include "flash_param.h"
#include "frame_builder.h"

static AlarmRecord_t alarm_buf[ALARM_MAX_RECORDS];
static uint8_t alarm_head = 0;   /* 写入位置 */
static uint8_t alarm_count = 0;  /* 当前记录数 */

/* ========== CRC32 (与 flash_param.c 一致) ========== */
static uint32_t alarm_calc_crc32(uint8_t *data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) crc = (crc >> 1) ^ (crc & 1 ? 0xEDB88320 : 0);
    }
    return ~crc;
}

/* ========== Flash 持久化 ========== */
void Alarm_SaveToFlash(void) {
    uint8_t buf[256] __attribute__((aligned(4)));
    uint32_t off = 0;

    /* Pack: head(1) + count(1) + reserved(2) + records(10*16) */
    buf[off++] = alarm_head;
    buf[off++] = alarm_count;
    buf[off++] = 0; buf[off++] = 0; /* reserved */
    for (int i = 0; i < ALARM_MAX_RECORDS; i++) {
        memcpy(&buf[off], &alarm_buf[i].utc, 4); off += 4;
        buf[off++] = alarm_buf[i].channel;
        buf[off++] = 0; buf[off++] = 0; buf[off++] = 0; /* padding */
        memcpy(&buf[off], &alarm_buf[i].threshold, 4); off += 4;
        memcpy(&buf[off], &alarm_buf[i].actual, 4); off += 4;
    }
    /* CRC32 over packed data */
    uint32_t crc = alarm_calc_crc32(buf, off);
    memcpy(&buf[off], &crc, 4); off += 4;

    /* Erase + Write */
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_page_erase(ALARM_FLASH_ADDR);
    while (fmc_flag_get(FMC_FLAG_BUSY) != RESET);
    fmc_flag_clear(FMC_FLAG_END);
    uint32_t words = (off + 3) / 4;
    uint32_t *src = (uint32_t *)buf;
    for (uint32_t i = 0; i < words; i++) {
        fmc_word_program(ALARM_FLASH_ADDR + i * 4, src[i]);
        while (fmc_flag_get(FMC_FLAG_BUSY) != RESET);
        fmc_flag_clear(FMC_FLAG_END);
    }
    fmc_lock();
}

static void Alarm_LoadFromFlash(void) {
    uint32_t off = 0;
    volatile uint8_t *flash = (volatile uint8_t *)ALARM_FLASH_ADDR;

    /* Read CRC first to check integrity */
    uint32_t stored_crc;
    /* Expected offset: 4 + 160 = 164 */
    uint32_t data_len = 4 + ALARM_MAX_RECORDS * 16; /* 4 + 160 = 164 */
    uint32_t calc_crc = alarm_calc_crc32((uint8_t *)flash, data_len);
    memcpy(&stored_crc, (void *)(flash + data_len), 4);

    if (calc_crc != stored_crc) {
        /* CRC mismatch → use defaults (all zeros) */
        alarm_head = 0;
        alarm_count = 0;
        return;
    }

    /* Unpack */
    alarm_head  = flash[off++];
    alarm_count = flash[off++];
    off += 2; /* skip reserved */
    if (alarm_count > ALARM_MAX_RECORDS) alarm_count = ALARM_MAX_RECORDS;
    if (alarm_head >= ALARM_MAX_RECORDS) alarm_head = 0;
    for (int i = 0; i < ALARM_MAX_RECORDS; i++) {
        memcpy(&alarm_buf[i].utc, (void *)(flash + off), 4); off += 4;
        alarm_buf[i].channel = flash[off++];
        off += 3; /* skip padding */
        memcpy(&alarm_buf[i].threshold, (void *)(flash + off), 4); off += 4;
        memcpy(&alarm_buf[i].actual, (void *)(flash + off), 4); off += 4;
    }
}

void Alarm_Init(void) {
    alarm_head = 0;
    alarm_count = 0;
    for (int i = 0; i < ALARM_MAX_RECORDS; i++) {
        alarm_buf[i].utc = 0;
        alarm_buf[i].channel = 0;
        alarm_buf[i].threshold = 0.0f;
        alarm_buf[i].actual = 0.0f;
    }
    /* 从Flash恢复告警记录 (赛题2.4节 持久化要求) */
    Alarm_LoadFromFlash();
}

/* 添加一条告警记录 (环形缓冲, 最多10条), 自动持久化到Flash */
static void alarm_add(uint8_t channel, float threshold, float actual) {
    alarm_buf[alarm_head].utc       = RTC_GetTime();
    alarm_buf[alarm_head].channel   = channel;
    alarm_buf[alarm_head].threshold = threshold;
    alarm_buf[alarm_head].actual    = actual;

    alarm_head = (alarm_head + 1) % ALARM_MAX_RECORDS;
    if (alarm_count < ALARM_MAX_RECORDS) alarm_count++;

    Alarm_SaveToFlash();  /* 每次告警触发后持久化 */
}

/* 检查阈值并触发告警 */
void Alarm_Check(float ch0, float ch1) {
    /* CH0 阈值检查 */
    if (g_param.ch0_threshold > 0.0f && ch0 > g_param.ch0_threshold) {
        alarm_add(0, g_param.ch0_threshold, ch0);
        /* 主动上报模式 → 立即输出字符串 */
        if (g_param.alarm_mode == 0x01) {
            Build_AlarmString(RTC_GetTime(), 0, g_param.ch0_threshold, ch0);
        }
    }

    /* CH1 阈值检查 */
    if (g_param.ch1_threshold > 0.0f && ch1 > g_param.ch1_threshold) {
        alarm_add(1, g_param.ch1_threshold, ch1);
        if (g_param.alarm_mode == 0x01) {
            Build_AlarmString(RTC_GetTime(), 1, g_param.ch1_threshold, ch1);
        }
    }
}

/* 查询告警记录 — 按时间倒序输出, 无告警回 "empty" (I-02) */
void Alarm_Query(void) {
    if (alarm_count == 0) {
        ProtoSendString("empty\r\n");
        return;
    }

    /* 按时间倒序: 最新在前 (环形缓冲: 最新在 head-1 位置) */
    for (int i = (int)alarm_count - 1; i >= 0; i--) {
        uint8_t idx = (alarm_head + ALARM_MAX_RECORDS - 1 - i) % ALARM_MAX_RECORDS;
        Build_AlarmString(alarm_buf[idx].utc,
                          alarm_buf[idx].channel,
                          alarm_buf[idx].threshold,
                          alarm_buf[idx].actual);
    }
}

/* 清除所有告警记录 (I-03), 同时清除Flash持久化 */
void Alarm_Clear(void) {
    alarm_head = 0;
    alarm_count = 0;
    Alarm_SaveToFlash();  /* 持久化清除操作 */
}

/* 设置告警模式: 0x01=主动上报, 0x02=仅存储 (I-04) */
void Alarm_SetMode(uint8_t mode) {
    g_param.alarm_mode = mode;
    Param_Save();
}
