/* SPDX-License-Identifier: MIT */

/**
 * @file auto_rpt.c
 * @brief Auto-report, alarm, heartbeat and application lifecycle management
 *
 * Central module for the instrument application runtime:
 *  - UTC time base with millisecond extrapolation
 *  - Per-channel value reading (ADC-scaled and PT100)
 *  - 12-byte big-endian payload builder (UTC + CH0 + CH1)
 *  - Auto-report scheduler with configurable interval
 *  - Alarm detection with rising-edge latching, ring-buffer storage,
 *    active-report push, query (newest-first), and clear with suppression
 *  - Startup heartbeat sequencer (1 s intervals, up to
 *    STARTUP_BEAT_COUNT messages)
 *  - AppCtrl_Init() one-time hardware / parameter initialisation
 *  - AppCtrl_Poll() periodic scheduler entry
 */

#include "app_core.h"
#include "packet_codec.h"
#include "wire_format.h"
#include "rs485_phy.h"
#include "mcu_cimc_gd32f470vet6.h"
#include "ota_uart.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Compile-time constants                                             */
/* ------------------------------------------------------------------ */

/** Number of startup heartbeats sent before the device goes idle */
#ifndef STARTUP_BEAT_COUNT
#define STARTUP_BEAT_COUNT  3
#endif

/** Maximum number of alarm records stored in the ring buffer */
#define ALARM_RECORD_MAX    10

/** Default UTC base (2026-01-01 00:00:00 = 1767225600) */
#define DEFAULT_UTC_BASE    1767225600UL

/** Default baud-rate code when the stored value is invalid */
#define DEFAULT_BAUD_CODE   0x13

/* ------------------------------------------------------------------ */
/*  Type definitions                                                   */
/* ------------------------------------------------------------------ */

/** Single alarm record stored in the ring buffer */
typedef struct
{
    uint32_t utc;       /**< UTC timestamp when alarm fired */
    uint8_t  channel;   /**< Channel index (0, 1, or 2)    */
    float    threshold;  /**< Configured threshold value     */
    float    actual;     /**< Measured value at trigger      */
} AlarmRecord_s;

/* ------------------------------------------------------------------ */
/*  Module-local state                                                 */
/* ------------------------------------------------------------------ */

/** UTC time reference point (seconds since epoch) */
static uint32_t m_TimeBaseUtc;

/** get_system_ms() reading captured when m_TimeBaseUtc was last set */
static uint32_t m_TimeBaseMs;

/** Number of startup heartbeats already transmitted */
static uint32_t m_StartupBeatCount;

/** Timestamp (ms) of the last startup heartbeat transmission */
static uint32_t m_LastStartupBeatMs;

/** 1 = auto-report active, 0 = stopped */
static uint8_t  m_AutoRptEnabled;

/** Interval in milliseconds between consecutive auto-reports */
static uint32_t m_RptIntervalMs;

/** Timestamp (ms) of the most recent auto-report transmission */
static uint32_t m_LastRptMs;

/** 1 = alarm active-report push enabled, 0 = silent */
static uint8_t  m_AlarmActiveReport;

/** Ring buffer holding up to ALARM_RECORD_MAX alarm records */
static AlarmRecord_s m_AlarmRecords[ALARM_RECORD_MAX];

/** Current number of valid records in the ring buffer (0 .. ALARM_RECORD_MAX) */
static uint32_t m_AlarmCount;

/** Write index for the next alarm record (wraps at ALARM_RECORD_MAX) */
static uint32_t m_AlarmWriteIdx;

/**
 * Rising-edge latch per channel.
 * 1 = channel value already exceeded threshold (alarm already fired).
 * Cleared to 0 when the value drops back below threshold.
 */
static uint8_t  m_AlarmOverThreshold[3];

/* ------------------------------------------------------------------ */
/*  Forward declarations (file-scope helpers)                          */
/* ------------------------------------------------------------------ */

static void format_alarm_line(char *buf, uint32_t buf_size,
                              const AlarmRecord_s *rec);

/* ------------------------------------------------------------------ */
/*  Time helpers                                                       */
/* ------------------------------------------------------------------ */

/**
 * @brief Obtain the current UTC time with millisecond extrapolation.
 * @return Current UTC timestamp in seconds since epoch.
 */
uint32_t Time_GetUtc(void)
{
    uint32_t elapsed_ms;
    uint32_t elapsed_s;

    elapsed_ms = get_system_ms() - m_TimeBaseMs;
    elapsed_s  = elapsed_ms / 1000U;

    return m_TimeBaseUtc + elapsed_s;
}

/**
 * @brief Set the UTC time reference.
 * @param utc New absolute UTC value (seconds since epoch).
 */
void Time_SetUtc(uint32_t utc)
{
    m_TimeBaseUtc = utc;
    m_TimeBaseMs  = get_system_ms();
}

/* ------------------------------------------------------------------ */
/*  Channel value reading                                              */
/* ------------------------------------------------------------------ */

/**
 * @brief Read the current engineering value for a measurement channel.
 *
 * Channels 0 and 1 are ADC raw values multiplied by their per-channel
 * calibration scale. Channel 2 returns the PT100 temperature directly.
 *
 * @param ch Channel index (0, 1, or 2).
 * @return Floating-point engineering value.
 */
float Ch_ReadValue(uint8_t ch)
{
    if (ch == 0)
    {
        return (float)g_AdcRaw[0] * Param_GetChScale(0);
    }
    else if (ch == 1)
    {
        return (float)g_AdcRaw[1] * Param_GetChScale(1);
    }
    else
    {
        return g_Pt100Temp;
    }
}

/* ------------------------------------------------------------------ */
/*  Report payload builder                                             */
/* ------------------------------------------------------------------ */

/**
 * @brief Build the 12-byte auto-report / response payload.
 *
 * Layout (big-endian / IEEE 754):
 *   [ 0 ..  3]  UTC timestamp (uint32, seconds)
 *   [ 4 ..  7]  Channel-0 value (float)
 *   [ 8 .. 11]  Channel-1 value (float)
 *
 * @param buf Output buffer, must be at least 12 bytes.
 */
void Rpt_BuildPayload(uint8_t *buf)
{
    Wire_PutU32BE(buf,     Time_GetUtc());
    Wire_PutFloatBE(buf + 4, Ch_ReadValue(0));
    Wire_PutFloatBE(buf + 8, Ch_ReadValue(1));
}

/* ------------------------------------------------------------------ */
/*  Auto-report accessors                                              */
/* ------------------------------------------------------------------ */

/**
 * @brief Query whether auto-report is currently enabled.
 * @return 1 if enabled, 0 otherwise.
 */
uint8_t AutoRpt_GetEnabled(void)
{
    return m_AutoRptEnabled;
}

/**
 * @brief Enable or disable auto-report.
 * @param en 1 to enable, 0 to disable.
 */
void AutoRpt_SetEnabled(uint8_t en)
{
    m_AutoRptEnabled = (en != 0) ? 1 : 0;
}

/**
 * @brief Get the configured auto-report interval.
 * @return Interval in milliseconds.
 */
uint32_t AutoRpt_GetInterval(void)
{
    return m_RptIntervalMs;
}

/**
 * @brief Set the auto-report interval.
 * @param ms Interval in milliseconds.
 */
void AutoRpt_SetInterval(uint32_t ms)
{
    m_RptIntervalMs = ms;
}

/**
 * @brief Get the timestamp of the most recent auto-report.
 * @return System-tick value (ms) recorded at last transmission.
 */
uint32_t AutoRpt_GetLastMs(void)
{
    return m_LastRptMs;
}

/**
 * @brief Record the timestamp of an auto-report transmission.
 * @param ms Current system-tick value (ms).
 */
void AutoRpt_SetLastMs(uint32_t ms)
{
    m_LastRptMs = ms;
}

/* ------------------------------------------------------------------ */
/*  Alarm helpers – formatting                                         */
/* ------------------------------------------------------------------ */

/**
 * @brief Format a single alarm record into the standard text line.
 *
 * The format is:
 *   "2026-01-01 HH:MM:SS | CHx | threshold | actual\r\n"
 *
 * @param buf      Destination character buffer.
 * @param buf_size Size of the destination buffer (bytes).
 * @param rec      Pointer to the alarm record to format.
 */
static void format_alarm_line(char *buf, uint32_t buf_size,
                              const AlarmRecord_s *rec)
{
    uint32_t sec_of_day;
    uint32_t h;
    uint32_t m;
    uint32_t s;

    sec_of_day = rec->utc % 86400U;
    h = sec_of_day / 3600U;
    m = (sec_of_day % 3600U) / 60U;
    s = sec_of_day % 60U;

    snprintf(buf, buf_size,
             "2026-01-01 %02lu:%02lu:%02lu | CH%u | %.2f | %.2f\r\n",
             h, m, s,
             (unsigned int)rec->channel,
             (double)rec->threshold,
             (double)rec->actual);
}

/* ------------------------------------------------------------------ */
/*  Alarm functions                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Get the active-report push flag.
 * @return 1 if active report is enabled, 0 otherwise.
 */
uint8_t Alarm_GetActiveReport(void)
{
    return m_AlarmActiveReport;
}

/**
 * @brief Set the active-report push flag.
 * @param en 1 to enable, 0 to disable.
 */
void Alarm_SetActiveReport(uint8_t en)
{
    m_AlarmActiveReport = (en != 0) ? 1 : 0;
}

/**
 * @brief Reset (clear) the over-threshold latch for a single channel.
 *
 * After this call the rising-edge detector will fire again the next
 * time the channel value exceeds the threshold.
 *
 * @param ch Channel index (0, 1, or 2).
 */
void Alarm_ResetOverThreshold(uint8_t ch)
{
    if (ch < 3)
    {
        m_AlarmOverThreshold[ch] = 0;
    }
}

/**
 * @brief Directly set the over-threshold latch for a single channel.
 * @param ch    Channel index (0, 1, or 2).
 * @param state 1 = latched, 0 = not latched.
 */
void Alarm_SetOverThreshold(uint8_t ch, uint8_t state)
{
    if (ch < 3)
    {
        m_AlarmOverThreshold[ch] = (state != 0) ? 1 : 0;
    }
}

/**
 * @brief Get the current over-threshold latch state for a channel.
 * @param ch Channel index (0, 1, or 2).
 * @return 1 if latched, 0 otherwise.
 */
uint8_t Alarm_GetOverThreshold(uint8_t ch)
{
    if (ch < 3)
    {
        return m_AlarmOverThreshold[ch];
    }
    return 0;
}

/**
 * @brief Check all channels for alarm conditions (rising-edge detect).
 *
 * For each channel the measured value is compared against the
 * configured threshold. If the value exceeds the threshold AND the
 * over-threshold latch is cleared, an alarm record is created,
 * stored in the ring buffer, and (if active-report is on) pushed
 * via Text_Tx. The latch is set to prevent repeated firing.
 * When the value drops back to or below the threshold the latch is
 * cleared automatically, re-arming the channel.
 */
void Alarm_Check(void)
{
    uint8_t  ch;
    float    value;
    float    threshold;

    for (ch = 0; ch < 3; ch++)
    {
        value     = Ch_ReadValue(ch);
        threshold = Param_GetThreshold(ch);

        if (value > threshold)
        {
            if (m_AlarmOverThreshold[ch] == 0)
            {
                /* Rising-edge: alarm triggered */
                m_AlarmOverThreshold[ch] = 1;

                /* Store in ring buffer */
                if (m_AlarmCount < ALARM_RECORD_MAX)
                {
                    m_AlarmCount++;
                }
                m_AlarmRecords[m_AlarmWriteIdx].utc       = Time_GetUtc();
                m_AlarmRecords[m_AlarmWriteIdx].channel   = ch;
                m_AlarmRecords[m_AlarmWriteIdx].threshold = threshold;
                m_AlarmRecords[m_AlarmWriteIdx].actual    = value;
                m_AlarmWriteIdx = (m_AlarmWriteIdx + 1) % ALARM_RECORD_MAX;

                /* Push alarm text if active-report is enabled */
                if (m_AlarmActiveReport != 0)
                {
                    uint32_t prev_idx;
                    char     line[80];

                    prev_idx = (m_AlarmWriteIdx == 0)
                               ? (ALARM_RECORD_MAX - 1)
                               : (m_AlarmWriteIdx - 1);
                    format_alarm_line(line, sizeof(line),
                                      &m_AlarmRecords[prev_idx]);
                    Text_Tx(line);
                }
            }
        }
        else
        {
            /* Value at or below threshold – re-arm the channel */
            m_AlarmOverThreshold[ch] = 0;
        }
    }
}

/**
 * @brief Query all stored alarm records, newest first.
 *
 * Output is sent as text lines via Text_Tx. When no records exist
 * the literal string "empty\r\n" is transmitted.
 */
void Alarm_Query(void)
{
    uint32_t i;
    uint32_t idx;
    char     line[80];

    if (m_AlarmCount == 0)
    {
        Text_Tx("empty\r\n");
        return;
    }

    for (i = 0; i < m_AlarmCount; i++)
    {
        idx = (m_AlarmWriteIdx - 1 - i + ALARM_RECORD_MAX) % ALARM_RECORD_MAX;
        format_alarm_line(line, sizeof(line), &m_AlarmRecords[idx]);
        Text_Tx(line);
    }
}

/**
 * @brief Clear all alarm records and suppress re-triggering.
 *
 * All stored records are discarded and the over-threshold latches
 * for all three channels are set to 1. This prevents any channel
 * from firing a new alarm until its value first drops below the
 * threshold (which clears the latch).
 */
void Alarm_Clear(void)
{
    m_AlarmCount   = 0;
    m_AlarmWriteIdx = 0;

    m_AlarmOverThreshold[0] = 1;
    m_AlarmOverThreshold[1] = 1;
    m_AlarmOverThreshold[2] = 1;
}

/**
 * @brief Reset all alarm runtime state to defaults.
 *
 * Equivalent to Alarm_Clear() but also resets the active-report flag.
 */
void Alarm_Reset(void)
{
    m_AlarmCount      = 0;
    m_AlarmWriteIdx    = 0;
    m_AlarmActiveReport = 0;

    m_AlarmOverThreshold[0] = 0;
    m_AlarmOverThreshold[1] = 0;
    m_AlarmOverThreshold[2] = 0;
}

/* ------------------------------------------------------------------ */
/*  Startup heartbeat accessors                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief Get the number of startup heartbeats already transmitted.
 * @return Transmitted count.
 */
uint32_t Beat_GetSent(void)
{
    return m_StartupBeatCount;
}

/**
 * @brief Set the number of startup heartbeats transmitted.
 *
 * Setting this to STARTUP_BEAT_COUNT (or higher) suppresses further
 * startup heartbeats. Called from AppCtrl_OnFrame() on any valid
 * command to fast-forward the startup sequence.
 *
 * @param count New transmitted count.
 */
void Beat_SetSent(uint32_t count)
{
    m_StartupBeatCount = count;
}

/**
 * @brief Get the timestamp of the last startup heartbeat.
 * @return System-tick value (ms).
 */
uint32_t Beat_GetLastMs(void)
{
    return m_LastStartupBeatMs;
}

/**
 * @brief Record the timestamp of a startup heartbeat transmission.
 * @param ms Current system-tick value (ms).
 */
void Beat_SetLastMs(uint32_t ms)
{
    m_LastStartupBeatMs = ms;
}

/* ------------------------------------------------------------------ */
/*  Heartbeat transmission                                             */
/* ------------------------------------------------------------------ */

/**
 * @brief Build and transmit a heartbeat frame.
 *
 * The frame is sent to the broadcast address with type PKT_TYPE_HB
 * and command CMD_HB (no payload).
 */
void Heartbeat_Send(void)
{
    Packet_s pkt;

    pkt.dev_id      = Param_GetDevId();
    pkt.type        = PKT_TYPE_HB;
    pkt.cmd         = CMD_HB;
    pkt.proto_ver   = PROTO_VER;
    pkt.payload     = NULL;
    pkt.payload_len = 0;

    Frame_Tx(&pkt);
}

/* ------------------------------------------------------------------ */
/*  Application initialisation                                         */
/* ------------------------------------------------------------------ */

/**
 * @brief One-time application initialisation.
 *
 * Loads persistent parameters from flash, seeds the UTC time base,
 * validates / defaults the baud-rate code, configures the RS485 PHY,
 * saves communication parameters for the bootloader, and restores
 * the DAC output.
 */
void AppCtrl_Init(void)
{
    uint8_t baud_code;

    /* Load persistent parameters from flash */
    Param_Load();

    /* Seed UTC base (2026-01-01 00:00:00) */
    m_TimeBaseUtc = DEFAULT_UTC_BASE;
    m_TimeBaseMs  = get_system_ms();

    /* Initialise runtime state */
    m_StartupBeatCount   = 0;
    m_LastStartupBeatMs  = 0;
    m_AutoRptEnabled     = 0;
    m_RptIntervalMs      = 1000;
    m_LastRptMs          = 0;
    m_AlarmActiveReport  = 0;
    m_AlarmCount         = 0;
    m_AlarmWriteIdx      = 0;

    m_AlarmOverThreshold[0] = 0;
    m_AlarmOverThreshold[1] = 0;
    m_AlarmOverThreshold[2] = 0;

    /* Validate baud code, default if invalid */
    baud_code = Param_GetBaudCode();
    if (Baud_FromCode(baud_code) == 0)
    {
        baud_code = DEFAULT_BAUD_CODE;
        Param_SetBaudCode(baud_code);
    }

    /* Apply baud rate to RS485 PHY */
    driver_rs485_set_baudrate(Baud_FromCode(baud_code));

    /* Persist communication parameters for bootloader access */
    ota_uart_save_comm_params();

    /* Restore DAC output to its last-saved value */
    dac_trigger_disable(DAC0, DAC_OUT0);
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, Param_GetDacValue());
}

/* ------------------------------------------------------------------ */
/*  Application poll                                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief Periodic application scheduler (call from main loop).
 *
 * Implements three modes in priority order:
 *  1. Auto-report active – sends periodic measurement reports at
 *     the configured interval, then runs Alarm_Check().
 *  2. Startup heartbeat – sends a heartbeat every 1 s until
 *     STARTUP_BEAT_COUNT messages have been transmitted, then runs
 *     Alarm_Check().
 *  3. Idle – runs Alarm_Check() only.
 */
void AppCtrl_Poll(void)
{
    uint32_t now;
    uint8_t  buf[12];
    Packet_s pkt;

    now = get_system_ms();

    if (m_AutoRptEnabled != 0)
    {
        /* === Auto-report mode === */
        if ((now - m_LastRptMs) >= m_RptIntervalMs)
        {
            Rpt_BuildPayload(buf);

            pkt.dev_id      = Param_GetDevId();
            pkt.type        = PKT_TYPE_HB;
            pkt.cmd         = CMD_HB;
            pkt.proto_ver   = PROTO_VER;
            pkt.payload     = buf;
            pkt.payload_len = 12;

            Frame_Tx(&pkt);
            m_LastRptMs = now;
        }

        Alarm_Check();
    }
    else if (m_StartupBeatCount < STARTUP_BEAT_COUNT)
    {
        /* === Startup heartbeat mode === */
        if ((now - m_LastStartupBeatMs) >= 1000U)
        {
            Heartbeat_Send();
            m_LastStartupBeatMs = now;
            m_StartupBeatCount++;
        }

        Alarm_Check();
    }
    else
    {
        /* === Idle mode === */
        Alarm_Check();
    }
}

/**
 * @brief Query whether the device is currently in auto-report mode.
 * @return 1 if auto-report is enabled, 0 otherwise.
 */
uint8_t AppCtrl_IsAutoRpt(void)
{
    return m_AutoRptEnabled;
}
