/* SPDX-License-Identifier: MIT */

/**
 * @file    protocol_defs.h
 * @brief   Wire-protocol constants, enumerations, and packet structure
 *          for the CIMC industrial-sensor communication stack.
 *
 * All numeric values on this page are dictated by the on-the-wire format
 * and MUST NOT be changed independently of the peer devices.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ------------------------------------------------------------------ */
/** @name Frame-delimiter constants                                   */
/* ------------------------------------------------------------------ */
/** Start-of-Frame marker (big-endian on wire) */
#define FRAME_SOF                0xA5B6U
/** End-of-Frame marker (big-endian on wire) */
#define FRAME_EOF                0xB6A5U

/* ------------------------------------------------------------------ */
/** @name Protocol version & addressing                               */
/* ------------------------------------------------------------------ */
/** Current protocol revision */
#define PROTO_VER                0x02U
/** Broadcast / wildcard device identifier */
#define BCAST_DEV_ID             0xFFFFU

/* ------------------------------------------------------------------ */
/** @name Frame-size boundaries                                       */
/* ------------------------------------------------------------------ */
/** Smallest valid binary frame (header + CRC + EOF, zero payload) */
#define PKT_MIN_BIN_LEN          13U
/** Largest payload a single frame may carry */
#define PKT_MAX_PAYLOAD          255U
/** Largest binary frame on wire */
#define PKT_MAX_BIN_LEN          (PKT_MIN_BIN_LEN + PKT_MAX_PAYLOAD)
/** Largest ASCII-hex frame on wire (2 hex chars per byte) */
#define PKT_MAX_ASC_LEN          (PKT_MAX_BIN_LEN * 2U)

/* ------------------------------------------------------------------ */
/** @enum  PktType_e
 *  @brief Packet type octet -- determines how the receiver interprets
 *         the frame.
 */
/* ------------------------------------------------------------------ */
typedef enum
{
    PKT_TYPE_CMD    = 0x01U,  /**< Master-to-slave command        */
    PKT_TYPE_RESP   = 0x02U,  /**< Slave-to-master response       */
    PKT_TYPE_HB     = 0x05U,  /**< Heartbeat / keep-alive         */
    PKT_TYPE_ERR    = 0xFFU   /**< Error / NACK indication        */
} PktType_e;

/* ------------------------------------------------------------------ */
/** @enum  CmdCode_e
 *  @brief Command identifiers -- grouped by subsystem.
 *  @note  Values are wire-format; do not renumber.
 */
/* ------------------------------------------------------------------ */
typedef enum
{
    /* ---- System (0x01xx) ---------------------------------------- */
    CMD_SYS_REBOOT          = 0x0101U,  /**< Reboot MCU                  */
    CMD_SYS_QUERY_FW_VER    = 0x0104U,  /**< Read firmware version       */
    CMD_SYS_SET_TIME        = 0x0105U,  /**< Write RTC time              */
    CMD_SYS_QUERY_TIME      = 0x0106U,  /**< Read RTC time               */
    CMD_SYS_QUERY_DEV_ID    = 0x0111U,  /**< Read device ID              */
    CMD_SYS_QUERY_BAUD      = 0x0112U,  /**< Read baudrate code          */
    CMD_SYS_SET_DEV_ID      = 0x01A1U,  /**< Write device ID             */
    CMD_SYS_SET_BAUD        = 0x01A2U,  /**< Write baudrate code         */

    /* ---- Data channel (0x02xx) ---------------------------------- */
    CMD_DATA_QUERY_CH0      = 0x0201U,  /**< Read channel-0 value        */
    CMD_DATA_QUERY_CH1      = 0x0202U,  /**< Read channel-1 value        */
    CMD_DATA_QUERY_PT100    = 0x0221U,  /**< Read PT100 temperature      */
    CMD_DATA_SET_CH0_RATIO  = 0x0241U,  /**< Write channel-0 ratio       */
    CMD_DATA_SET_CH1_RATIO  = 0x0242U,  /**< Write channel-1 ratio       */
    CMD_DATA_SET_REPORT_IVAL = 0x0261U, /**< Set auto-report interval    */

    /* ---- Control (0x03xx) --------------------------------------- */
    CMD_CTRL_SET_DAC        = 0x0301U,  /**< Set DAC output              */
    CMD_CTRL_START_AUTO     = 0x0302U,  /**< Begin auto-reporting        */
    CMD_CTRL_STOP_AUTO      = 0x0303U,  /**< Halt auto-reporting         */
    CMD_CTRL_SLEEP          = 0x03AAU,  /**< Enter low-power sleep       */

    /* ---- Configuration / thresholds (0x04xx) -------------------- */
    CMD_CFG_READ_THRESHOLDS = 0x0400U,  /**< Read all threshold values   */
    CMD_CFG_READ_CH0_THR    = 0x0401U,  /**< Read channel-0 threshold    */
    CMD_CFG_READ_CH1_THR    = 0x0402U,  /**< Read channel-1 threshold    */
    CMD_CFG_READ_CH2_THR    = 0x0403U,  /**< Read channel-2 threshold    */
    CMD_CFG_WRITE_CH0_THR   = 0x0411U,  /**< Write channel-0 threshold   */
    CMD_CFG_WRITE_CH1_THR   = 0x0412U,  /**< Write channel-1 threshold   */
    CMD_CFG_WRITE_CH2_THR   = 0x0413U,  /**< Write channel-2 threshold   */

    /* ---- Firmware upgrade (0x05xx) ------------------------------ */
    CMD_OTA_REQ             = 0x0501U,  /**< Request OTA upgrade         */
    CMD_OTA_PREPARE         = 0x0502U,  /**< Prepare for OTA data        */
    CMD_OTA_EXEC            = 0x0503U,  /**< Execute OTA / jump to app   */

    /* ---- Alarm subsystem (0x06xx) ------------------------------- */
    CMD_ALM_ENABLE          = 0x0601U,  /**< Enable alarm monitoring     */
    CMD_ALM_QUERY           = 0x0602U,  /**< Query alarm state           */
    CMD_ALM_CLEAR           = 0x0603U,  /**< Clear latched alarms        */

    /* ---- Special ------------------------------------------------- */
    CMD_HB                  = 0x8888U,  /**< Heartbeat command           */
    CMD_BCAST_DISCOVER      = 0xFFFFU,  /**< Broadcast discovery probe   */
    CMD_ERR_GENERIC         = 0xEEEEU   /**< Generic error indicator     */
} CmdCode_e;

/* ------------------------------------------------------------------ */
/** @enum  PacketErr_e
 *  @brief Return codes for the packet-decode state machine.
 */
/* ------------------------------------------------------------------ */
typedef enum
{
    PKT_ERR_OK      = 0,    /**< Decode succeeded                     */
    PKT_ERR_NULL,           /**< NULL pointer argument                */
    PKT_ERR_SHORT,          /**< Input shorter than minimum frame     */
    PKT_ERR_LONG,           /**< Input exceeds maximum frame          */
    PKT_ERR_HEX,            /**< Invalid hex character or odd length  */
    PKT_ERR_MARKER,         /**< SOF or EOF marker mismatch           */
    PKT_ERR_VER,            /**< Protocol-version mismatch            */
    PKT_ERR_LEN,            /**< Declared length inconsistent         */
    PKT_ERR_CRC             /**< CRC-16 integrity check failed        */
} PacketErr_e;

/* ------------------------------------------------------------------ */
/** @struct Packet_s
 *  @brief  Decoded representation of one protocol frame.
 */
/* ------------------------------------------------------------------ */
typedef struct
{
    uint16_t dev_id;                            /**< Source / destination device ID    */
    uint8_t  type;                              /**< Packet type (see @ref PktType_e)  */
    uint16_t cmd;                               /**< Command code (see @ref CmdCode_e) */
    uint8_t  payload_len;                       /**< Payload byte count (0..255)        */
    uint8_t  proto_ver;                         /**< Protocol version from frame        */
    uint8_t  payload[PKT_MAX_PAYLOAD];          /**< Payload buffer                    */
} Packet_s;

#ifdef __cplusplus
}
#endif
