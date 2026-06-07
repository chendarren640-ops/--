/* SPDX-License-Identifier: MIT */

/**
 * @file param_store.c
 * @brief Persistent parameter storage with CRC32 integrity verification.
 */

#include "app_core.h"
#include <stddef.h>
#include <string.h>

/* ================================================================== */
/** @name Constants                                                     */
/* ================================================================== */

#define PARAM_MAGIC              0x43494D43UL
#define PARAM_VERSION            0x00010001UL
#define PARAM_FILE_PATH          "cimc_param.bin"
#define DEVICE_ID_DEFAULT        0x0001U
#define BAUD_CODE_DEFAULT        0x13U

/* ================================================================== */
/** @name Persistent storage structure                                  */
/* ================================================================== */

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint16_t device_id;
    uint8_t  baudrate_code;
    uint8_t  reserved;
    float    ch_scale[2];
    float    threshold[3];
    uint16_t dac_value;
    uint16_t reserved2;
    uint32_t crc32;
} ParamStore_s;

/* ================================================================== */
/** @name Module-local state                                            */
/* ================================================================== */

static uint16_t m_DevId        = DEVICE_ID_DEFAULT;
static uint8_t  m_BaudCode     = BAUD_CODE_DEFAULT;
static float    m_ChScale[2]   = {1.0f, 1.0f};
static float    m_Threshold[3] = {3000.0f, 3000.0f, 80.0f};
static uint16_t m_DacValue     = 0U;

/* ================================================================== */
/** @name CRC32 (Ethernet polynomial)                                   */
/* ================================================================== */

static uint32_t Crc32_Update(uint32_t crc, const void *data, uint32_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t i;

    crc = ~crc;
    while (len-- > 0U)
    {
        crc ^= *p++;
        for (i = 0U; i < 8U; i++)
        {
            if ((crc & 1U) != 0U)
            {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

static uint32_t Param_CalcCrc(const ParamStore_s *param)
{
    return Crc32_Update(0U, param, (uint32_t)offsetof(ParamStore_s, crc32));
}

/* ================================================================== */
/** @name Internal helpers                                              */
/* ================================================================== */

static void Param_SetDefaults(void)
{
    m_DevId        = DEVICE_ID_DEFAULT;
    m_BaudCode     = BAUD_CODE_DEFAULT;
    m_ChScale[0]   = 1.0f;
    m_ChScale[1]   = 1.0f;
    m_Threshold[0] = 3000.0f;
    m_Threshold[1] = 3000.0f;
    m_Threshold[2] = 80.0f;
    m_DacValue     = 0U;
}

/* ================================================================== */
/** @name Public API -- Persistence                                      */
/* ================================================================== */

void Param_Save(void)
{
    ParamStore_s param;

    memset(&param, 0, sizeof(param));
    param.magic         = PARAM_MAGIC;
    param.version       = PARAM_VERSION;
    param.device_id     = m_DevId;
    param.baudrate_code = m_BaudCode;
    param.ch_scale[0]   = m_ChScale[0];
    param.ch_scale[1]   = m_ChScale[1];
    param.threshold[0]  = m_Threshold[0];
    param.threshold[1]  = m_Threshold[1];
    param.threshold[2]  = m_Threshold[2];
    param.dac_value     = m_DacValue;
    param.crc32         = Param_CalcCrc(&param);

    (void)flash_lfs_write_file(PARAM_FILE_PATH, &param, sizeof(param));
}

void Param_Load(void)
{
    ParamStore_s param;
    int len;

    Param_SetDefaults();

    len = flash_lfs_read_file(PARAM_FILE_PATH, &param, sizeof(param));
    if (len != (int)sizeof(param))
    {
        return;
    }
    if ((param.magic != PARAM_MAGIC) ||
        (param.version != PARAM_VERSION) ||
        (param.crc32 != Param_CalcCrc(&param)))
    {
        return;
    }

    m_DevId        = param.device_id;
    m_BaudCode     = param.baudrate_code;
    m_ChScale[0]   = param.ch_scale[0];
    m_ChScale[1]   = param.ch_scale[1];
    m_Threshold[0] = param.threshold[0];
    m_Threshold[1] = param.threshold[1];
    m_Threshold[2] = param.threshold[2];
    m_DacValue     = param.dac_value;
}

/* ================================================================== */
/** @name Baud-rate codec                                               */
/* ================================================================== */

uint32_t Baud_FromCode(uint8_t code)
{
    switch (code)
    {
    case 0x11U: return 4800U;
    case 0x12U: return 9600U;
    case 0x13U: return 19200U;
    case 0x14U: return 115200U;
    default:    return 0U;
    }
}

/* ================================================================== */
/** @name Parameter accessors                                           */
/* ================================================================== */

uint16_t Param_GetDevId(void)          { return m_DevId; }
void     Param_SetDevId(uint16_t id)   { m_DevId = id; }
uint8_t  Param_GetBaudCode(void)       { return m_BaudCode; }
void     Param_SetBaudCode(uint8_t c)  { m_BaudCode = c; }
float    Param_GetChScale(uint8_t ch)  { return m_ChScale[ch]; }
void     Param_SetChScale(uint8_t ch, float s) { m_ChScale[ch] = s; }
float    Param_GetThreshold(uint8_t ch) { return m_Threshold[ch]; }
void     Param_SetThreshold(uint8_t ch, float t) { m_Threshold[ch] = t; }
uint16_t Param_GetDacValue(void)       { return m_DacValue; }
void     Param_SetDacValue(uint16_t v) { m_DacValue = v; }

uint16_t AppCtrl_GetDevId(void)
{
    return Param_GetDevId();
}

/* ================================================================== */
/** @name Big-endian wire-format codec                                  */
/* ================================================================== */

void Wire_PutU16BE(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value >> 8);
    out[1] = (uint8_t)(value & 0xFFU);
}

void Wire_PutU32BE(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)(value & 0xFFU);
}

uint32_t Wire_GetU32BE(const uint8_t *in)
{
    return (((uint32_t)in[0] << 24) |
            ((uint32_t)in[1] << 16) |
            ((uint32_t)in[2] << 8) |
            (uint32_t)in[3]);
}

void Wire_PutFloatBE(uint8_t *out, float value)
{
    union { float f; uint32_t u; } cvt;
    cvt.f = value;
    Wire_PutU32BE(out, cvt.u);
}

float Wire_GetFloatBE(const uint8_t *in)
{
    union { float f; uint32_t u; } cvt;
    cvt.u = Wire_GetU32BE(in);
    return cvt.f;
}
