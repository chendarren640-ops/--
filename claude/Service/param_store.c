/* SPDX-License-Identifier: MIT */

/**
 * @file param_store.c
 * @brief Persistent parameter storage with CRC32 integrity verification.
 *
 * Stores device identity, calibration scales, alarm thresholds, DAC output
 * value, and baud-rate code in a LittleFS file.  A magic number and version
 * field guard against loading garbage, and a CRC32 protects against
 * partially-written flash pages.
 */

#include "app_core.h"
#include <stddef.h>
#include <string.h>

/* ================================================================== */
/** @name Constants                                                     */
/* ================================================================== */

/** Magic value written at the head of every valid parameter block. */
#define PARAM_MAGIC              0x43494D43UL
/** Parameter-block format version; increment when layout changes. */
#define PARAM_VERSION            0x00010001UL
/** LittleFS file path used for the parameter image. */
#define PARAM_FILE_PATH          "cimc_param.bin"
/** Factory-default device identifier. */
#define DEVICE_ID_DEFAULT        0x0001U
/** Factory-default baud-rate code (0x13 = 19200 baud). */
#define BAUD_CODE_DEFAULT        0x13U

/* ================================================================== */
/** @name Persistent storage structure                                  */
/* ================================================================== */

/**
 * @brief On-flash layout of the parameter block.
 *
 * Field order and sizes MUST NOT change without bumping PARAM_VERSION,
 * otherwise old images will fail the CRC check and be discarded.
 */
typedef struct
{
    uint32_t magic;          /**< PARAM_MAGIC */
    uint32_t version;        /**< PARAM_VERSION */
    uint16_t device_id;      /**< 16-bit bus address */
    uint8_t  baudrate_code;  /**< Encoded baud rate (see Baud_FromCode) */
    uint8_t  reserved;       /**< Padding to keep 32-bit alignment */
    float    ch_scale[2];    /**< Channel-0/1 calibration ratio */
    float    threshold[3];   /**< Alarm threshold per channel */
    uint16_t dac_value;      /**< 12-bit DAC output code */
    uint16_t reserved2;      /**< Padding */
    uint32_t crc32;          /**< CRC32 over all preceding fields */
} ParamStore_s;

/* ================================================================== */
/** @name Module-local state                                            */
/* ================================================================== */

static uint16_t m_DevId       = DEVICE_ID_DEFAULT;
static uint8_t  m_BaudCode    = BAUD_CODE_DEFAULT;
static float    m_ChScale[2]  = {1.0f, 1.0f};
static float    m_Threshold[3] = {3000.0f, 3000.0f, 80.0f};
static uint16_t m_DacValue    = 0U;

/* ================================================================== */
/** @name CRC32 (Ethernet polynomial)                                   */
/* ================================================================== */

/**
 * @brief Streaming CRC32 update using the standard Ethernet polynomial.
 * @param[in] crc   Previous CRC value (use 0U for the first chunk).
 * @param[in] data  Pointer to the byte buffer.
 * @param[in] len   Number of bytes to process.
 * @return    Updated CRC32 value.
 */
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

/**
 * @brief Compute the CRC32 of a parameter block's header fields
 *        (everything up to, but not including, the crc32 member).
 * @param[in] param  Pointer to a populated ParamStore_s.
 * @return    CRC32 checksum.
 */
static uint32_t Param_CalcCrc(const ParamStore_s *param)
{
    return Crc32_Update(0U, param, (uint32_t)offsetof(ParamStore_s, crc32));
}

/* ================================================================== */
/** @name Internal helpers                                              */
/* ================================================================== */

/**
 * @brief Restore all module-level variables to their factory defaults.
 */
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
/** @name Public API — Persistence                                      */
/* ================================================================== */

/**
 * @brief Serialise current parameters and write them to the flash
 *        filesystem with a valid CRC32.
 */
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

/**
 * @brief Load parameters from the flash filesystem.
 *
 * If the stored image is missing, truncated, or fails the
 * magic/version/CRC check, factory defaults are kept.
 */
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
/** @name Public API — Baud-rate codec                                  */
/* ================================================================== */

/**
 * @brief Convert a protocol baud-rate code to an actual bit rate.
 * @param[in] code  Encoded baud rate (0x11–0x14 valid).
 * @return    Baud rate in Hz, or 0U if the code is unrecognised.
 */
uint32_t Baud_FromCode(uint8_t code)
{
    switch (code)
    {
    case 0x11U:
        return 4800U;
    case 0x12U:
        return 9600U;
    case 0x13U:
        return 19200U;
    case 0x14U:
        return 115200U;
    default:
        return 0U;
    }
}

/* ================================================================== */
/** @name Public API — Parameter accessors                              */
/* ================================================================== */

uint16_t Param_GetDevId(void)
{
    return m_DevId;
}

void Param_SetDevId(uint16_t id)
{
    m_DevId = id;
}

uint8_t Param_GetBaudCode(void)
{
    return m_BaudCode;
}

void Param_SetBaudCode(uint8_t code)
{
    m_BaudCode = code;
}

float Param_GetChScale(uint8_t channel)
{
    return m_ChScale[channel];
}

void Param_SetChScale(uint8_t channel, float scale)
{
    m_ChScale[channel] = scale;
}

float Param_GetThreshold(uint8_t channel)
{
    return m_Threshold[channel];
}

void Param_SetThreshold(uint8_t channel, float threshold)
{
    m_Threshold[channel] = threshold;
}

uint16_t Param_GetDacValue(void)
{
    return m_DacValue;
}

void Param_SetDacValue(uint16_t value)
{
    m_DacValue = value;
}

/**
 * @brief Convenience wrapper that exposes the device ID to legacy callers.
 * @return Current 16-bit device ID.
 */
uint16_t AppCtrl_GetDevId(void)
{
    return Param_GetDevId();
}

/* ================================================================== */
/** @name Public API — Big-endian wire-format codec                     */
/* ================================================================== */

/**
 * @brief Encode a 16-bit unsigned integer in big-endian byte order.
 * @param[out] out    Destination buffer (at least 2 bytes).
 * @param[in]  value  Value to encode.
 */
void Wire_PutU16BE(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value >> 8);
    out[1] = (uint8_t)(value & 0xFFU);
}

/**
 * @brief Encode a 32-bit unsigned integer in big-endian byte order.
 * @param[out] out    Destination buffer (at least 4 bytes).
 * @param[in]  value  Value to encode.
 */
void Wire_PutU32BE(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)(value & 0xFFU);
}

/**
 * @brief Decode a 32-bit unsigned integer from a big-endian buffer.
 * @param[in] in  Source buffer (at least 4 bytes).
 * @return    Decoded 32-bit value.
 */
uint32_t Wire_GetU32BE(const uint8_t *in)
{
    return (((uint32_t)in[0] << 24) |
            ((uint32_t)in[1] << 16) |
            ((uint32_t)in[2] << 8) |
            (uint32_t)in[3]);
}

/**
 * @brief Encode an IEEE-754 single-precision float in big-endian order.
 * @param[out] out    Destination buffer (at least 4 bytes).
 * @param[in]  value  Float value to encode.
 */
void Wire_PutFloatBE(uint8_t *out, float value)
{
    union
    {
        float    f;
        uint32_t u;
    } cvt;

    cvt.f = value;
    Wire_PutU32BE(out, cvt.u);
}

/**
 * @brief Decode an IEEE-754 single-precision float from a big-endian buffer.
 * @param[in] in  Source buffer (at least 4 bytes).
 * @return    Decoded float value.
 */
float Wire_GetFloatBE(const uint8_t *in)
{
    union
    {
        float    f;
        uint32_t u;
    } cvt;

    cvt.u = Wire_GetU32BE(in);
    return cvt.f;
}
