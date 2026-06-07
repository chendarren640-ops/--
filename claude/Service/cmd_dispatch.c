/* SPDX-License-Identifier: MIT */

/**
 * @file cmd_dispatch.c
 * @brief Protocol command dispatcher and PHY-level frame transmission.
 *
 * Parses incoming ASCII frames, routes recognised commands to the
 * appropriate handler, and transmits response frames and alarm text
 * over the RS485 physical layer.
 */

#include "app_core.h"
#include "packet_codec.h"
#include "wire_format.h"
#include "rs485_phy.h"
#include "mcu_cimc_gd32f470vet6.h"
#include "ota_uart.h"
#include "oled_app.h"
#include <string.h>

/* ================================================================== */
/** @name Local constants                                               */
/* ================================================================== */

/** Firmware version components (reported by QUERY_FW_VERSION). */
#define FW_VER_MAJOR             2U
#define FW_VER_MINOR             0U
#define FW_VER_PATCH             1U
#define FW_VER_BUILD             0U

/** Deep-sleep RTC wakeup timeout in seconds. */
#define SLEEP_WAKEUP_SEC         10U

/* ================================================================== */
/** @name PHY-level frame transmission (public)                         */
/* ================================================================== */

/**
 * @brief Serialise a protocol frame to its ASCII representation and
 *        transmit it via the RS485 driver.
 * @param[in] frame_ptr  Pointer to a populated cimc_frame_t.
 */
void Frame_Tx(const void *frame_ptr)
{
    const cimc_frame_t *frame = (const cimc_frame_t *)frame_ptr;
    char   ascii[CIMC_FRAME_MAX_ASCII_LEN];
    size_t written = 0U;

    if (cimc_frame_build_ascii(frame, ascii, sizeof(ascii), &written))
    {
        driver_rs485_send_bytes((const uint8_t *)ascii, written);
    }
}

/**
 * @brief Transmit a raw null-terminated string via RS485.
 * @param[in] text  String to send; NULL is silently ignored.
 */
void Text_Tx(const char *text)
{
    if (text != NULL)
    {
        driver_rs485_send_bytes((const uint8_t *)text, strlen(text));
    }
}

/* ================================================================== */
/** @name Internal response helpers (static)                            */
/* ================================================================== */

/**
 * @brief Transmit an error frame for the given command.
 * @param[in] dev_id   Device ID to embed in the error frame.
 * @param[in] command  Command code that caused the error.
 */
static void Cmd_SendErr(uint16_t dev_id, uint16_t command)
{
    cimc_frame_t frame;

    if (cimc_frame_make_error(dev_id, command, NULL, 0U, &frame))
    {
        Frame_Tx(&frame);
    }
}

/**
 * @brief Transmit a success (OK) response frame.
 * @param[in] dev_id   Device ID.
 * @param[in] command  Original command code being acknowledged.
 */
static void Cmd_SendOk(uint16_t dev_id, uint16_t command)
{
    cimc_frame_t frame;

    if (cimc_frame_make_ok(dev_id, command, &frame))
    {
        Frame_Tx(&frame);
    }
}

/**
 * @brief Transmit a response frame carrying a binary payload.
 * @param[in] dev_id       Device ID.
 * @param[in] command      Original command code.
 * @param[in] payload      Binary payload buffer.
 * @param[in] payload_len  Payload size in bytes.
 */
static void Cmd_SendResp(uint16_t     dev_id,
                         uint16_t     command,
                         const uint8_t *payload,
                         uint8_t      payload_len)
{
    cimc_frame_t frame;

    if (cimc_frame_make_response(dev_id, command, payload, payload_len, &frame))
    {
        Frame_Tx(&frame);
    }
}

/* ================================================================== */
/** @name Command dispatcher                                            */
/* ================================================================== */

/**
 * @brief Route a parsed protocol request frame to the handler for its
 *        command code.
 *
 * Handles device-ID filtering, auto-report gate, startup-heartbeat
 * suppression, and all defined protocol commands.
 * @param[in] request_ptr  Pointer to a fully-parsed cimc_frame_t.
 */
void Cmd_Dispatch(const void *request_ptr)
{
    const cimc_frame_t *request = (const cimc_frame_t *)request_ptr;
    uint8_t     payload[12];

    /* --- Address filtering ---------------------------------------- */
    if (request->device_id != Param_GetDevId() &&
        request->device_id != CIMC_DEVICE_ID_BROADCAST)
    {
        return;
    }

    /* --- Auto-report gate: only STOP_AUTO_REPORT is accepted
     *     while the periodic reporter is active. ------------------ */
    if ((AutoRpt_GetEnabled() != 0U) &&
        (request->command != CIMC_CMD_STOP_AUTO_REPORT))
    {
        return;
    }

    /*
     * The upper PC has clearly seen us once we reply to any command —
     * stop the startup heartbeat so its 1 s tick will not get spliced
     * onto a B-02/F-01 reply.
     */
    Beat_SetSent(STARTUP_BEAT_COUNT);

    switch (request->command)
    {
    /* -------------------------------------------------------------- */
    case CIMC_CMD_BROADCAST_DISCOVERY:
        Heartbeat_Send();
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_REBOOT:
        /*
         * Show "Bootloader" before the reset. The SSD1306 keeps its
         * GRAM across a warm reset, so the BootLoader region displays
         * "Bootloader" (req. 2.6) without needing its own OLED driver;
         * the APP repaints IDLE after it boots.
         */
        oled_printf(0, 2, "Bootloader");
        Cmd_SendOk(Param_GetDevId(), request->command);
        delay_ms(20);
        NVIC_SystemReset();
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_QUERY_FW_VERSION:
        payload[0] = FW_VER_MAJOR;
        payload[1] = FW_VER_MINOR;
        payload[2] = FW_VER_PATCH;
        payload[3] = FW_VER_BUILD;
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 4U);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_SET_TIME:
        if (request->payload_len != 4U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        Time_SetUtc(Wire_GetU32BE(request->payload));
        Cmd_SendOk(Param_GetDevId(), request->command);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_QUERY_TIME:
        Wire_PutU32BE(payload, Time_GetUtc());
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 4U);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_QUERY_DEVICE_ID:
        Wire_PutU16BE(payload, Param_GetDevId());
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 2U);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_QUERY_BAUDRATE:
        payload[0] = Param_GetBaudCode();
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 1U);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_SET_DEVICE_ID:
        if (request->payload_len != 2U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        {
            uint16_t new_id = (uint16_t)(((uint16_t)request->payload[0] << 8) |
                                          request->payload[1]);
            if ((new_id == 0x0000U) || (new_id == CIMC_DEVICE_ID_BROADCAST))
            {
                Cmd_SendErr(Param_GetDevId(), request->command);
                break;
            }
            Param_SetDevId(new_id);
            Param_Save();
            /*
             * Mirror the new ID into the BootLoader param page so an
             * upgrade after L-01 (ID != 0x0001) is still addressed
             * correctly in BL (N-02/N-03).
             */
            (void)ota_uart_save_comm_params(Param_GetDevId(), Param_GetBaudCode());
            Cmd_SendOk(Param_GetDevId(), request->command);
        }
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_SET_BAUDRATE:
        if ((request->payload_len != 1U) ||
            (Baud_FromCode(request->payload[0]) == 0U))
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        Param_SetBaudCode(request->payload[0]);
        Param_Save();
        (void)ota_uart_save_comm_params(Param_GetDevId(), Param_GetBaudCode());
        Cmd_SendOk(Param_GetDevId(), request->command);
        /* Let the ACK go out at the old baud rate before switching. */
        delay_ms(20);
        driver_rs485_set_baudrate(Baud_FromCode(Param_GetBaudCode()));
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_QUERY_CH0:
        Wire_PutFloatBE(payload, Ch_ReadValue(0U));
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 4U);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_QUERY_CH1:
        Wire_PutFloatBE(payload, Ch_ReadValue(1U));
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 4U);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_QUERY_PT100:
        Wire_PutFloatBE(payload, Ch_ReadValue(2U));
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 4U);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_SET_CH0_RATIO:
        if (request->payload_len != 4U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        Param_SetChScale(0U, Wire_GetFloatBE(request->payload));
        Param_Save();
        Cmd_SendOk(Param_GetDevId(), request->command);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_SET_CH1_RATIO:
        if (request->payload_len != 4U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        Param_SetChScale(1U, Wire_GetFloatBE(request->payload));
        Param_Save();
        Cmd_SendOk(Param_GetDevId(), request->command);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_SET_REPORT_INTERVAL:
        if (request->payload_len != 1U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        if (request->payload[0] == 0x01U)
        {
            AutoRpt_SetInterval(1000U);
        }
        else if (request->payload[0] == 0x02U)
        {
            AutoRpt_SetInterval(3000U);
        }
        else if (request->payload[0] == 0x03U)
        {
            AutoRpt_SetInterval(5000U);
        }
        else
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        Cmd_SendOk(Param_GetDevId(), request->command);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_SET_DAC:
        if (request->payload_len != 2U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        {
            uint16_t dac = (uint16_t)((((uint16_t)request->payload[0] << 8) |
                                        request->payload[1]) & 0x0FFFU);
            Param_SetDacValue(dac);
        }
        dac_trigger_disable(DAC0, DAC_OUT0);
        dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, Param_GetDacValue());
        /*
         * Persist DAC so F-02 still sees CH1 ~ ratio*DAC after reboot.
         * Without this DAC resets to 0 and CH1 drops to noise floor,
         * masking ratio persistence.
         */
        Param_Save();
        Cmd_SendOk(Param_GetDevId(), request->command);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_START_AUTO_REPORT:
        Rpt_BuildPayload(payload);
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 12U);
        AutoRpt_SetEnabled(1U);
        AutoRpt_SetLastMs(get_system_ms());
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_STOP_AUTO_REPORT:
        AutoRpt_SetEnabled(0U);
        Cmd_SendOk(Param_GetDevId(), request->command);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_SLEEP:
        if (request->payload_len != 0U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        Cmd_SendOk(Param_GetDevId(), request->command);
        AutoRpt_SetEnabled(0U);
        delay_ms(20);
        bsp_enter_deepsleep_rtc_wakeup(SLEEP_WAKEUP_SEC);
        Text_Tx("instrument wakeup\r\n");
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_UPGRADE_REQUEST:
        if (request->payload_len != 0U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        if (ota_uart_request_bootloader_upgrade() == 0U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        /*
         * Same trick as reboot: leave "Bootloader" on the OLED so the
         * upgrade-wait window (N-01) shows the required status without
         * a BootLoader OLED driver.
         */
        oled_printf(0, 2, "Bootloader");
        Cmd_SendOk(Param_GetDevId(), request->command);
        delay_ms(20);
        NVIC_SystemReset();
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_READ_THRESHOLDS:
        Wire_PutFloatBE(&payload[0], Param_GetThreshold(0U));
        Wire_PutFloatBE(&payload[4], Param_GetThreshold(1U));
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 8U);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_READ_CH0_THRESHOLD:
        Wire_PutFloatBE(payload, Param_GetThreshold(0U));
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 4U);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_READ_CH1_THRESHOLD:
        Wire_PutFloatBE(payload, Param_GetThreshold(1U));
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 4U);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_READ_CH2_THRESHOLD:
        Wire_PutFloatBE(payload, Param_GetThreshold(2U));
        Cmd_SendResp(Param_GetDevId(), request->command, payload, 4U);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_WRITE_CH0_THRESHOLD:
        if (request->payload_len != 4U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        Param_SetThreshold(0U, Wire_GetFloatBE(request->payload));
        Param_Save();
        Cmd_SendOk(Param_GetDevId(), request->command);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_WRITE_CH1_THRESHOLD:
        if (request->payload_len != 4U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        Param_SetThreshold(1U, Wire_GetFloatBE(request->payload));
        Param_Save();
        Cmd_SendOk(Param_GetDevId(), request->command);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_WRITE_CH2_THRESHOLD:
        if (request->payload_len != 4U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        Param_SetThreshold(2U, Wire_GetFloatBE(request->payload));
        Param_Save();
        Cmd_SendOk(Param_GetDevId(), request->command);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_ALARM_ENABLE:
        if (request->payload_len != 1U)
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        if (request->payload[0] == 0x01U)
        {
            Alarm_SetActiveReport(1U);
        }
        else if (request->payload[0] == 0x02U)
        {
            Alarm_SetActiveReport(0U);
        }
        else
        {
            Cmd_SendErr(Param_GetDevId(), request->command);
            break;
        }
        Alarm_ResetOverThreshold(0U);
        Alarm_ResetOverThreshold(1U);
        Alarm_ResetOverThreshold(2U);
        Cmd_SendOk(Param_GetDevId(), request->command);
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_ALARM_QUERY:
        Alarm_Query();
        break;

    /* -------------------------------------------------------------- */
    case CIMC_CMD_ALARM_CLEAR:
        Alarm_Clear();
        Cmd_SendOk(Param_GetDevId(), request->command);
        break;

    /* -------------------------------------------------------------- */
    default:
        /*
         * Per 4.5.7(2), unknown command field returns EEEE error
         * frame. The script expects FF EEEE here even though the
         * protocol also lists "original command word".
         */
        Cmd_SendErr(Param_GetDevId(), CIMC_CMD_ERROR_GENERIC);
        break;
    }
}

/* ================================================================== */
/** @name Public API — Frame ingestion                                 */
/* ================================================================== */

/**
 * @brief Process an incoming ASCII protocol frame.
 *
 * Parses the frame, validates its type, and dispatches to Cmd_Dispatch().
 * Malformed frames receive an error response.
 * @param[in] ascii  Pointer to the ASCII frame string.
 * @param[in] len    Number of valid bytes in @p ascii.
 */
void AppCtrl_OnFrame(const char *ascii, size_t len)
{
    cimc_frame_t request;

    if (cimc_frame_parse_ascii(ascii, len, &request) != CIMC_PARSE_OK)
    {
        Cmd_SendErr(Param_GetDevId(), CIMC_CMD_ERROR_GENERIC);
        return;
    }

    if (request.type != CIMC_FRAME_TYPE_COMMAND &&
        request.type != CIMC_FRAME_TYPE_HEARTBEAT)
    {
        Cmd_SendErr(Param_GetDevId(), request.command);
        return;
    }

    Cmd_Dispatch(&request);
}
