/* SPDX-License-Identifier: MIT */

/**
 * @file cmd_dispatch.c
 * @brief Command dispatch and incoming frame handler
 *
 * Decodes incoming command packets and dispatches to the appropriate
 * handler. Implements address filtering, auto-report gating, and
 * startup heartbeat fast-forward on any valid command.
 */

#include "app_core.h"
#include "packet_codec.h"
#include "wire_format.h"
#include "rs485_phy.h"
#include "mcu_cimc_gd32f470vet6.h"
#include "ota_uart.h"
#include "oled_app.h"
#include <string.h>

/** Firmware major version */
#define FW_VER_MAJOR  2
/** Firmware minor version */
#define FW_VER_MINOR  0
/** Firmware patch version */
#define FW_VER_PATCH  1
/** Firmware build number */
#define FW_VER_BUILD  0

/** Duration in seconds the device sleeps before RTC wakeup */
#define SLEEP_WAKEUP_SEC  10

/** Maximum size needed for an ASCII-encoded frame buffer */
#define FRAME_BUF_SIZE  PKT_MAX_ASC_LEN

/**
 * @brief Transmit a fully-formed packet over the RS485 bus.
 * @param pkt Pointer to the packet to encode and send.
 */
void Frame_Tx(const Packet_s *pkt)
{
    static uint8_t buf[FRAME_BUF_SIZE];
    uint32_t len;

    len = Packet_Encode(pkt, buf, sizeof(buf));
    if (len > 0)
    {
        driver_rs485_send_bytes(buf, len);
    }
}

/**
 * @brief Transmit a raw null-terminated text string over RS485.
 * @param text Pointer to the null-terminated string to send.
 */
void Text_Tx(const char *text)
{
    driver_rs485_send_bytes((const uint8_t *)text, (uint32_t)strlen(text));
}

/**
 * @brief Build and transmit an error response for the given request.
 * @param request Pointer to the original request packet.
 */
static void Cmd_SendErr(const Packet_s *request)
{
    Packet_s err;

    Packet_MakeErr(&err, request, CMD_ERR_GENERIC);
    Frame_Tx(&err);
}

/**
 * @brief Build and transmit a success (OK) response for the given request.
 * @param request Pointer to the original request packet.
 */
static void Cmd_SendOk(const Packet_s *request)
{
    Packet_s resp;

    Packet_MakeOk(&resp, request);
    Frame_Tx(&resp);
}

/**
 * @brief Build and transmit a response carrying a binary payload.
 * @param request Pointer to the original request packet.
 * @param payload Byte buffer holding the response data.
 * @param len    Length of the payload in bytes.
 */
static void Cmd_SendResp(const Packet_s *request, const uint8_t *payload, uint32_t len)
{
    Packet_s resp;

    Packet_MakeResp(&resp, request, payload, len);
    Frame_Tx(&resp);
}

/**
 * @brief Dispatch a decoded command packet to its handler.
 *
 * Runs inside AppCtrl_OnFrame() after address filtering and gating.
 * Each handler independently validates its payload length and content.
 *
 * @param request Pointer to the decoded, validated request packet.
 */
static void Cmd_Dispatch(const Packet_s *request)
{
    uint8_t  payload[12];
    uint32_t val_u32;
    uint16_t val_u16;
    float    val_f;

    switch (request->cmd)
    {
    /* ------------------------------------------------------------------ */
    /*  Broadcast / Heartbeat                                              */
    /* ------------------------------------------------------------------ */
    case CMD_BCAST_DISCOVER:
        Heartbeat_Send();
        break;

    /* ------------------------------------------------------------------ */
    /*  System commands                                                    */
    /* ------------------------------------------------------------------ */
    case CMD_SYS_REBOOT:
        oled_printf(0, 2, "Bootloader");
        Cmd_SendOk(request);
        delay_ms(20);
        NVIC_SystemReset();
        break;

    case CMD_SYS_QUERY_FW_VER:
        payload[0] = FW_VER_MAJOR;
        payload[1] = FW_VER_MINOR;
        payload[2] = FW_VER_PATCH;
        payload[3] = FW_VER_BUILD;
        Cmd_SendResp(request, payload, 4);
        break;

    case CMD_SYS_SET_TIME:
        if (request->payload_len != 4)
        {
            Cmd_SendErr(request);
            break;
        }
        val_u32 = Wire_GetU32BE(request->payload);
        Time_SetUtc(val_u32);
        Cmd_SendOk(request);
        break;

    case CMD_SYS_QUERY_TIME:
        Wire_PutU32BE(payload, Time_GetUtc());
        Cmd_SendResp(request, payload, 4);
        break;

    case CMD_SYS_QUERY_DEV_ID:
        Wire_PutU16BE(payload, Param_GetDevId());
        Cmd_SendResp(request, payload, 2);
        break;

    case CMD_SYS_QUERY_BAUD:
        payload[0] = Param_GetBaudCode();
        Cmd_SendResp(request, payload, 1);
        break;

    case CMD_SYS_SET_DEV_ID:
        if (request->payload_len != 2)
        {
            Cmd_SendErr(request);
            break;
        }
        val_u16 = ((uint16_t)request->payload[0] << 8) | request->payload[1];
        if ((val_u16 == 0) || (val_u16 == BCAST_DEV_ID))
        {
            Cmd_SendErr(request);
            break;
        }
        Param_SetDevId(val_u16);
        Param_Save();
        ota_uart_save_comm_params();
        Cmd_SendOk(request);
        break;

    case CMD_SYS_SET_BAUD:
        if (request->payload_len != 1)
        {
            Cmd_SendErr(request);
            break;
        }
        if (Baud_FromCode(request->payload[0]) == 0)
        {
            Cmd_SendErr(request);
            break;
        }
        Param_SetBaudCode(request->payload[0]);
        Param_Save();
        ota_uart_save_comm_params();
        Cmd_SendOk(request);
        delay_ms(20);
        driver_rs485_set_baudrate(Baud_FromCode(request->payload[0]));
        break;

    /* ------------------------------------------------------------------ */
    /*  Data query / read commands                                         */
    /* ------------------------------------------------------------------ */
    case CMD_DATA_QUERY_CH0:
        Wire_PutFloatBE(payload, Ch_ReadValue(0));
        Cmd_SendResp(request, payload, 4);
        break;

    case CMD_DATA_QUERY_CH1:
        Wire_PutFloatBE(payload, Ch_ReadValue(1));
        Cmd_SendResp(request, payload, 4);
        break;

    case CMD_DATA_QUERY_PT100:
        Wire_PutFloatBE(payload, Ch_ReadValue(2));
        Cmd_SendResp(request, payload, 4);
        break;

    /* ------------------------------------------------------------------ */
    /*  Data configuration commands                                        */
    /* ------------------------------------------------------------------ */
    case CMD_DATA_SET_CH0_RATIO:
        if (request->payload_len != 4)
        {
            Cmd_SendErr(request);
            break;
        }
        val_f = Wire_GetFloatBE(request->payload);
        Param_SetChScale(0, val_f);
        Param_Save();
        Cmd_SendOk(request);
        break;

    case CMD_DATA_SET_CH1_RATIO:
        if (request->payload_len != 4)
        {
            Cmd_SendErr(request);
            break;
        }
        val_f = Wire_GetFloatBE(request->payload);
        Param_SetChScale(1, val_f);
        Param_Save();
        Cmd_SendOk(request);
        break;

    case CMD_DATA_SET_REPORT_IVAL:
        if (request->payload_len != 1)
        {
            Cmd_SendErr(request);
            break;
        }
        if (request->payload[0] == 0x01)
        {
            val_u32 = 1000;
        }
        else if (request->payload[0] == 0x02)
        {
            val_u32 = 3000;
        }
        else if (request->payload[0] == 0x03)
        {
            val_u32 = 5000;
        }
        else
        {
            Cmd_SendErr(request);
            break;
        }
        AutoRpt_SetInterval(val_u32);
        Cmd_SendOk(request);
        break;

    /* ------------------------------------------------------------------ */
    /*  Control commands                                                   */
    /* ------------------------------------------------------------------ */
    case CMD_CTRL_SET_DAC:
        if (request->payload_len != 2)
        {
            Cmd_SendErr(request);
            break;
        }
        val_u16 = ((uint16_t)request->payload[0] << 8) | request->payload[1];
        val_u16 &= 0x0FFF;
        Param_SetDacValue(val_u16);
        dac_trigger_disable(DAC0, DAC_OUT0);
        dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, val_u16);
        Param_Save();
        Cmd_SendOk(request);
        break;

    case CMD_CTRL_START_AUTO:
        Rpt_BuildPayload(payload);
        Cmd_SendResp(request, payload, 12);
        AutoRpt_SetEnabled(1);
        AutoRpt_SetLastMs(get_system_ms());
        break;

    case CMD_CTRL_STOP_AUTO:
        AutoRpt_SetEnabled(0);
        Cmd_SendOk(request);
        break;

    case CMD_CTRL_SLEEP:
        if (request->payload_len != 0)
        {
            Cmd_SendErr(request);
            break;
        }
        Cmd_SendOk(request);
        AutoRpt_SetEnabled(0);
        delay_ms(20);
        bsp_enter_deepsleep_rtc_wakeup(SLEEP_WAKEUP_SEC);
        Text_Tx("instrument wakeup\r\n");
        break;

    /* ------------------------------------------------------------------ */
    /*  OTA / firmware update                                              */
    /* ------------------------------------------------------------------ */
    case CMD_OTA_REQ:
        if (request->payload_len != 0)
        {
            Cmd_SendErr(request);
            break;
        }
        ota_uart_request_bootloader_upgrade();
        oled_printf(0, 2, "Bootloader");
        Cmd_SendOk(request);
        delay_ms(20);
        NVIC_SystemReset();
        break;

    /* ------------------------------------------------------------------ */
    /*  Alarm threshold configuration                                      */
    /* ------------------------------------------------------------------ */
    case CMD_CFG_READ_THRESHOLDS:
        Wire_PutFloatBE(payload,     Param_GetThreshold(0));
        Wire_PutFloatBE(payload + 4, Param_GetThreshold(1));
        Cmd_SendResp(request, payload, 8);
        break;

    case CMD_CFG_READ_CH0_THR:
        Wire_PutFloatBE(payload, Param_GetThreshold(0));
        Cmd_SendResp(request, payload, 4);
        break;

    case CMD_CFG_READ_CH1_THR:
        Wire_PutFloatBE(payload, Param_GetThreshold(1));
        Cmd_SendResp(request, payload, 4);
        break;

    case CMD_CFG_READ_CH2_THR:
        Wire_PutFloatBE(payload, Param_GetThreshold(2));
        Cmd_SendResp(request, payload, 4);
        break;

    case CMD_CFG_WRITE_CH0_THR:
        if (request->payload_len != 4)
        {
            Cmd_SendErr(request);
            break;
        }
        val_f = Wire_GetFloatBE(request->payload);
        Param_SetThreshold(0, val_f);
        Param_Save();
        Cmd_SendOk(request);
        break;

    case CMD_CFG_WRITE_CH1_THR:
        if (request->payload_len != 4)
        {
            Cmd_SendErr(request);
            break;
        }
        val_f = Wire_GetFloatBE(request->payload);
        Param_SetThreshold(1, val_f);
        Param_Save();
        Cmd_SendOk(request);
        break;

    case CMD_CFG_WRITE_CH2_THR:
        if (request->payload_len != 4)
        {
            Cmd_SendErr(request);
            break;
        }
        val_f = Wire_GetFloatBE(request->payload);
        Param_SetThreshold(2, val_f);
        Param_Save();
        Cmd_SendOk(request);
        break;

    /* ------------------------------------------------------------------ */
    /*  Alarm management                                                   */
    /* ------------------------------------------------------------------ */
    case CMD_ALM_ENABLE:
        if (request->payload_len != 1)
        {
            Cmd_SendErr(request);
            break;
        }
        if (request->payload[0] == 0x01)
        {
            Alarm_SetActiveReport(1);
        }
        else if (request->payload[0] == 0x02)
        {
            Alarm_SetActiveReport(0);
        }
        else
        {
            Cmd_SendErr(request);
            break;
        }
        Alarm_ResetOverThreshold(0);
        Alarm_ResetOverThreshold(1);
        Alarm_ResetOverThreshold(2);
        Cmd_SendOk(request);
        break;

    case CMD_ALM_QUERY:
        Alarm_Query();
        break;

    case CMD_ALM_CLEAR:
        Alarm_Clear();
        Cmd_SendOk(request);
        break;

    /* ------------------------------------------------------------------ */
    /*  Unknown / unhandled command                                        */
    /* ------------------------------------------------------------------ */
    default:
        Cmd_SendErr(request);
        break;
    }
}

/**
 * @brief Entry point for all incoming decoded frames.
 *
 * Performs address filtering (own device ID or broadcast only) and
 * auto-report gating (blocks commands while auto-report is active,
 * except CMD_CTRL_STOP_AUTO). On any valid command the startup
 * heartbeat counter is fast-forwarded to suppress further startup
 * heartbeats.
 *
 * @param frame Pointer to the decoded incoming packet (may be NULL).
 */
void AppCtrl_OnFrame(const Packet_s *frame)
{
    if (frame == NULL)
    {
        return;
    }

    /* Address filtering: only our ID or broadcast address */
    if ((frame->dev_id != Param_GetDevId()) && (frame->dev_id != BCAST_DEV_ID))
    {
        return;
    }

    /* Auto-report gate: block all commands except STOP_AUTO while active */
    if (AutoRpt_GetEnabled() && (frame->cmd != CMD_CTRL_STOP_AUTO))
    {
        return;
    }

    /* Any valid command suppresses further startup heartbeats */
    Beat_SetSent(STARTUP_BEAT_COUNT);

    Cmd_Dispatch(frame);
}
