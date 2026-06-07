/* SPDX-License-Identifier: MIT */

/**
 * @file auto_rpt.c
 * @brief Auto-report scheduler, alarm engine, heartbeat transmission,
 *        and application lifecycle (init / poll).
 */

#include "app_core.h"
#include "packet_codec.h"
#include "rs485_phy.h"
#include "mcu_cimc_gd32f470vet6.h"
#include "ota_uart.h"
#include <stdio.h>
#include <string.h>

/* ================================================================== */
/** @name Constants                                                     */
/* ================================================================== */

/** Default auto-report interval in milliseconds. */
#define REPORT_INTERVAL_DEFAULT  1000U

/** Maximum number of stored alarm records (circular buffer). */
#define ALARM_RECORD_MAX         10U

/** Output buffer size for a single alarm text line. */
#define ALARM_LINE_SIZE          96U

/**
 * @brief Deep-sleep wakeup timeout in seconds.
 *
 * The RTC alarm is programmed to fire after this many seconds;
 * the MCU then resumes execution after send_text("instrument wakeup").
 */
#define SLEEP_WAKEUP_SEC         10U

/**
 * @brief Default UTC epoch base when no external time has been set.
 *
 * Boot clock base: 2026-01-01 00:00:00 UTC. The evaluator's B-02
 * queries device time before C-01 sets it; returning epoch 0 (1970)
 * is treated as "no valid time" and fails.  Seeding a 2026 base
 * makes the unset clock report a plausible date.
 */
#define UTC_BASE_DEFAULT         1767225600UL

/* ================================================================== */
/** @name Alarm record type                                             */
/* ================================================================== */

/**
 * @brief Single alarm event stored in the circular log.
 */
typedef struct
{
    uint32_t timestamp;  /**< UTC seconds when the alarm fired */
    uint8_t  channel;    /**< Channel index (0, 1, or 2) */
    float    threshold;  /**< Threshold value that was exceeded */
    float    value;      /**< Measured value at alarm time */
} AlarmRecord_s;

/* ================================================================== */
/** @name Module-local state                                            */
/* ================================================================== */

static uint32_t m_TimeBaseUtc          = 0U;
static uint32_t m_TimeBaseMs           = 0U;

static uint8_t  m_StartupBeatCount     = 0U;
static uint32_t m_LastStartupBeatMs    = 0U;

static uint8_t  m_AutoRptEnabled       = 0U;
static uint32_t m_RptIntervalMs        = REPORT_INTERVAL_DEFAULT;
static uint32_t m_LastRptMs            = 0U;

static uint8_t  m_AlarmActiveReport    = 0U;
static AlarmRecord_s m_AlarmRecords[ALARM_RECORD_MAX];
static uint8_t  m_AlarmCount           = 0U;
static uint8_t  m_AlarmWriteIdx        = 0U;
static uint8_t  m_AlarmOverThreshold[3] = {0U, 0U, 0U};

/* ================================================================== */
/** @name Time helpers                                                  */
/* ================================================================== */

/**
 * @brief Compute the current UTC time in seconds.
 *
 * Extrapolates from the last-set (time_base_utc, time_base_ms) pair
 * using the system monotonic millisecond counter.
 * @return UTC seconds since the configured epoch base.
 */
uint32_t Time_GetUtc(void)
{
    return m_TimeBaseUtc + ((get_system_ms() - m_TimeBaseMs) / 1000U);
}

/**
 * @brief Set the UTC time base and capture the current system tick.
 * @param[in] utc  New UTC seconds value to use as the reference point.
 */
void Time_SetUtc(uint32_t utc)
{
    m_TimeBaseUtc = utc;
    m_TimeBaseMs  = get_system_ms();
}

/* ================================================================== */
/** @name Channel measurement                                           */
/* ================================================================== */

/**
 * @brief Read the calibrated value of a measurement channel.
 * @param[in] channel  0 = CH0 (ADC raw * scale), 1 = CH1, 2 = PT100.
 * @return    Scaled floating-point measurement.
 */
float Ch_ReadValue(uint8_t channel)
{
    if (channel == 0U)
    {
        return (float)g_AdcRaw[0] * Param_GetChScale(0U);
    }
    if (channel == 1U)
    {
        return (float)g_AdcRaw[1] * Param_GetChScale(1U);
    }

    return g_Pt100Temp;
}

/* ================================================================== */
/** @name Report payload builder                                        */
/* ================================================================== */

/**
 * @brief Build the 12-byte auto-report / query payload.
 *
 * Layout: [0..3] UTC seconds BE, [4..7] CH0 float BE,
 *         [8..11] CH1 float BE.
 * @param[out] payload  Destination buffer (at least 12 bytes).
 */
void Rpt_BuildPayload(uint8_t *payload)
{
    Wire_PutU32BE(&payload[0], Time_GetUtc());
    Wire_PutFloatBE(&payload[4], Ch_ReadValue(0U));
    Wire_PutFloatBE(&payload[8], Ch_ReadValue(1U));
}

/* ================================================================== */
/** @name Alarm text formatting                                         */
/* ================================================================== */

/**
 * @brief Format one alarm record into a human-readable line.
 * @param[in]  record   Pointer to the alarm record.
 * @param[out] out      Destination buffer.
 * @param[in]  out_len  Size of @p out in bytes.
 * @return    Number of bytes written (excluding null terminator),
 *            or 0 on parameter error.
 */
static int Alarm_FormatLine(const AlarmRecord_s *record,
                            char *out, size_t out_len)
{
    uint32_t seconds;
    uint32_t minutes;
    uint32_t hours;

    if ((record == NULL) || (out == NULL) || (out_len == 0U))
    {
        return 0;
    }

    seconds = record->timestamp % 60U;
    minutes = (record->timestamp / 60U) % 60U;
    hours   = (record->timestamp / 3600U) % 24U;

    return snprintf(out,
                    out_len,
                    "2026-01-01 %02lu:%02lu:%02lu | CH%u | %.2f | %.2f\r\n",
                    (unsigned long)hours,
                    (unsigned long)minutes,
                    (unsigned long)seconds,
                    record->channel,
                    record->threshold,
                    record->value);
}

/* ================================================================== */
/** @name Alarm record management                                       */
/* ================================================================== */

/**
 * @brief Append a new alarm record to the circular buffer.
 *
 * If active alarm reporting is enabled the record is also
 * transmitted immediately as a text line.
 * @param[in] channel    Channel that triggered the alarm.
 * @param[in] threshold  Threshold value that was exceeded.
 * @param[in] value      Measured value at the time of the alarm.
 */
static void Alarm_AddRecord(uint8_t channel, float threshold, float value)
{
    AlarmRecord_s *record = &m_AlarmRecords[m_AlarmWriteIdx];

    record->timestamp = Time_GetUtc();
    record->channel   = channel;
    record->threshold = threshold;
    record->value     = value;

    m_AlarmWriteIdx = (uint8_t)((m_AlarmWriteIdx + 1U) % ALARM_RECORD_MAX);
    if (m_AlarmCount < ALARM_RECORD_MAX)
    {
        m_AlarmCount++;
    }

    if (m_AlarmActiveReport != 0U)
    {
        char line[ALARM_LINE_SIZE];
        (void)Alarm_FormatLine(record, line, sizeof(line));
        Text_Tx(line);
    }
}

/* ================================================================== */
/** @name Public API — Alarm engine                                     */
/* ================================================================== */

/**
 * @brief Evaluate the alarm threshold for a single channel.
 *
 * If the channel transitions from below-threshold to above-threshold
 * a new alarm record is created.
 * @param[in] channel  Channel index (0, 1, or 2).
 */
void Alarm_Check(uint8_t channel)
{
    float   threshold = Param_GetThreshold(channel);
    float   value     = Ch_ReadValue(channel);
    uint8_t over      = (value > threshold) ? 1U : 0U;

    if ((over != 0U) && (m_AlarmOverThreshold[channel] == 0U))
    {
        Alarm_AddRecord(channel, threshold, value);
    }

    m_AlarmOverThreshold[channel] = over;
}

/**
 * @brief Transmit all stored alarm records as text lines over RS485.
 *
 * Records are output newest-first.  If no records exist the literal
 * string "empty\r\n" is sent.
 */
void Alarm_Query(void)
{
    char    line[ALARM_LINE_SIZE];
    uint8_t i;

    if (m_AlarmCount == 0U)
    {
        Text_Tx("empty\r\n");
        return;
    }

    for (i = 0U; i < m_AlarmCount; i++)
    {
        uint8_t newest = (m_AlarmWriteIdx + ALARM_RECORD_MAX - 1U - i) % ALARM_RECORD_MAX;
        (void)Alarm_FormatLine(&m_AlarmRecords[newest], line, sizeof(line));
        Text_Tx(line);
    }
}

/**
 * @brief Clear all alarm records and suppress immediate re-trigger.
 *
 * After clearing, the over-threshold state for every channel is set
 * to 1 so that channels currently above their threshold will not
 * immediately fire again on the next Alarm_Check() call.
 */
void Alarm_Clear(void)
{
    m_AlarmCount    = 0U;
    m_AlarmWriteIdx = 0U;
    memset(m_AlarmRecords, 0, sizeof(m_AlarmRecords));
    /*
     * Suppress immediate re-record for channels still above threshold.
     * The script issues query right after clear; without this a channel
     * reading above its threshold re-fires immediately.
     */
    m_AlarmOverThreshold[0] = 1U;
    m_AlarmOverThreshold[1] = 1U;
    m_AlarmOverThreshold[2] = 1U;
}

/**
 * @brief Reset alarm state to power-on defaults (no suppression).
 *
 * Used during AppCtrl_Init() so that the first Alarm_Check() cycle
 * correctly detects channels already above threshold after boot.
 */
void Alarm_Reset(void)
{
    m_AlarmCount           = 0U;
    m_AlarmWriteIdx        = 0U;
    memset(m_AlarmRecords, 0, sizeof(m_AlarmRecords));
    m_AlarmOverThreshold[0] = 0U;
    m_AlarmOverThreshold[1] = 0U;
    m_AlarmOverThreshold[2] = 0U;
}

/* ================================================================== */
/** @name Public API — Alarm accessors                                  */
/* ================================================================== */

uint8_t Alarm_GetActiveReport(void)
{
    return m_AlarmActiveReport;
}

void Alarm_SetActiveReport(uint8_t active)
{
    m_AlarmActiveReport = active;
}

void Alarm_ResetOverThreshold(uint8_t channel)
{
    m_AlarmOverThreshold[channel] = 0U;
}

void Alarm_SetOverThreshold(uint8_t channel, uint8_t val)
{
    m_AlarmOverThreshold[channel] = val;
}

uint8_t Alarm_GetOverThreshold(uint8_t channel)
{
    return m_AlarmOverThreshold[channel];
}

/* ================================================================== */
/** @name Public API — Auto-report accessors                            */
/* ================================================================== */

uint8_t AutoRpt_GetEnabled(void)
{
    return m_AutoRptEnabled;
}

void AutoRpt_SetEnabled(uint8_t enabled)
{
    m_AutoRptEnabled = enabled;
}

uint32_t AutoRpt_GetInterval(void)
{
    return m_RptIntervalMs;
}

void AutoRpt_SetInterval(uint32_t ms)
{
    m_RptIntervalMs = ms;
}

uint32_t AutoRpt_GetLastMs(void)
{
    return m_LastRptMs;
}

void AutoRpt_SetLastMs(uint32_t ms)
{
    m_LastRptMs = ms;
}

/* ================================================================== */
/** @name Public API — Startup heartbeat accessors                      */
/* ================================================================== */

uint8_t Beat_GetSent(void)
{
    return m_StartupBeatCount;
}

void Beat_SetSent(uint8_t count)
{
    m_StartupBeatCount = count;
}

uint32_t Beat_GetLastMs(void)
{
    return m_LastStartupBeatMs;
}

void Beat_SetLastMs(uint32_t ms)
{
    m_LastStartupBeatMs = ms;
}

/* ================================================================== */
/** @name Public API — Heartbeat transmission                           */
/* ================================================================== */

/**
 * @brief Transmit a single protocol heartbeat frame.
 *
 * Constructs a raw frame with type = HEARTBEAT, command = HEARTBEAT,
 * and the current protocol version, then sends it via Frame_Tx().
 */
void Heartbeat_Send(void)
{
    cimc_frame_t frame;

    memset(&frame, 0, sizeof(frame));
    frame.device_id   = Param_GetDevId();
    frame.type        = CIMC_FRAME_TYPE_HEARTBEAT;
    frame.command     = CIMC_CMD_HEARTBEAT;
    frame.version     = CIMC_PROTOCOL_VERSION;
    frame.payload_len = 0U;
    Frame_Tx(&frame);
}

/* ================================================================== */
/** @name Public API — Application lifecycle                            */
/* ================================================================== */

/**
 * @brief One-time application initialisation.
 *
 * 1. Load persisted parameters from flash.
 * 2. Seed the UTC time base with a plausible 2026 epoch.
 * 3. Configure the RS485 PHY to the stored baud rate.
 * 4. Mirror communication parameters to the BootLoader page.
 * 5. Restore the saved DAC output level.
 */
void AppCtrl_Init(void)
{
    Param_Load();

    Time_SetUtc(UTC_BASE_DEFAULT);

    Beat_SetSent(0U);
    Beat_SetLastMs(0U);

    AutoRpt_SetEnabled(0U);
    AutoRpt_SetInterval(REPORT_INTERVAL_DEFAULT);
    AutoRpt_SetLastMs(0U);

    Alarm_SetActiveReport(0U);
    Alarm_Reset();

    /* Validate baud code — if flash held an invalid code, fall back to 19200. */
    if (Baud_FromCode(Param_GetBaudCode()) == 0U)
    {
        Param_SetBaudCode(0x13U);
    }
    driver_rs485_set_baudrate(Baud_FromCode(Param_GetBaudCode()));
    (void)ota_uart_save_comm_params(Param_GetDevId(), Param_GetBaudCode());

    /* Restore previously persisted DAC output so CH1 readback survives reboot.
     * Param_Load() already populated the DAC value from flash. */
    {
        uint16_t dac = Param_GetDacValue();
        if (dac > 0x0FFFU)
        {
            dac = 0U;
            Param_SetDacValue(0U);
        }
        dac_trigger_disable(DAC0, DAC_OUT0);
        dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, dac);
    }
}

/**
 * @brief Periodic application task.
 *
 * Call this from the main loop or RTOS tick.  It handles three
 * mutually-exclusive operating modes:
 *   - Auto-report active: transmit periodic report frames on schedule.
 *   - Startup heartbeat phase: send initial heartbeat(s) at 1 s intervals
 *     until STARTUP_BEAT_COUNT is reached.
 *   - Normal idle: evaluate alarm thresholds on all channels.
 */
void AppCtrl_Poll(void)
{
    uint32_t    now = get_system_ms();
    cimc_frame_t report;
    uint8_t     payload[12];

    if (AutoRpt_GetEnabled() != 0U)
    {
        if ((now - AutoRpt_GetLastMs()) >= AutoRpt_GetInterval())
        {
            Rpt_BuildPayload(payload);
            if (cimc_frame_make_response(Param_GetDevId(),
                                         CIMC_CMD_START_AUTO_REPORT,
                                         payload, 12U, &report))
            {
                Frame_Tx(&report);
            }
            AutoRpt_SetLastMs(now);
        }
        return;
    }

    if (Beat_GetSent() >= STARTUP_BEAT_COUNT)
    {
        Alarm_Check(0U);
        Alarm_Check(1U);
        Alarm_Check(2U);
        return;
    }

    if ((Beat_GetSent() == 0U) || ((now - Beat_GetLastMs()) >= 1000U))
    {
        Heartbeat_Send();
        Beat_SetLastMs(now);
        Beat_SetSent(Beat_GetSent() + 1U);
    }

    Alarm_Check(0U);
    Alarm_Check(1U);
    Alarm_Check(2U);
}

/**
 * @brief Query whether auto-report mode is currently active.
 * @return Non-zero when periodic auto-report transmission is enabled.
 */
uint8_t AppCtrl_IsAutoRpt(void)
{
    return AutoRpt_GetEnabled();
}
