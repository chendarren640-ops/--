/* SPDX-License-Identifier: MIT */

/**
 * @file    packet_codec.c
 * @brief   Frame encode / decode engine.  Translates between the
 *          binary packet layout and the ASCII-hex wire format,
 *          validating SOF/EOF markers, protocol version, length,
 *          and CRC-16 along the way.
 */

#include "packet_codec.h"
#include "wire_format.h"
#include <string.h>

/* ================================================================== */
/*  Internal helpers -- big-endian 16-bit accessors                    */
/* ================================================================== */

/**
 * @brief  Read a big-endian uint16_t from a byte buffer.
 */
static uint16_t Buf_GetU16(const uint8_t *buf)
{
    return (uint16_t)(((uint16_t)buf[0] << 8) | (uint16_t)buf[1]);
}

/**
 * @brief  Write a uint16_t into a byte buffer in big-endian order.
 */
static void Buf_SetU16(uint8_t *buf, uint16_t val)
{
    buf[0] = (uint8_t)(val >> 8);
    buf[1] = (uint8_t)(val & 0xFFU);
}

/* ================================================================== */
/*  Packet_Decode  --  ASCII-hex  -->  Packet_s                        */
/* ================================================================== */

PacketErr_e Packet_Decode(const char *ascii, size_t ascii_len, Packet_s *pkt)
{
    uint8_t  raw[PKT_MAX_BIN_LEN];
    size_t   raw_len;
    uint8_t  plen;
    uint16_t crc_rx;
    uint16_t crc_ok;

    /* ---- Sanity checks ------------------------------------------ */
    if ((ascii == NULL) || (pkt == NULL))
    {
        return PKT_ERR_NULL;
    }
    if (ascii_len < ((size_t)PKT_MIN_BIN_LEN * 2U))
    {
        return PKT_ERR_SHORT;
    }
    if (ascii_len > (size_t)PKT_MAX_ASC_LEN)
    {
        return PKT_ERR_LONG;
    }
    if ((ascii_len & 1U) != 0U)
    {
        return PKT_ERR_HEX;
    }

    /* ---- Hex-to-binary conversion ------------------------------- */
    if (!HexStr_ToBin(ascii, ascii_len, raw, sizeof(raw)))
    {
        return PKT_ERR_HEX;
    }

    raw_len = ascii_len / 2U;

    /* ---- Frame markers ------------------------------------------ */
    if ((Buf_GetU16(&raw[0]) != FRAME_SOF)
        || (Buf_GetU16(&raw[raw_len - 2U]) != FRAME_EOF))
    {
        return PKT_ERR_MARKER;
    }

    /* ---- Payload length, version, total-length consistency ------ */
    plen = raw[7];
    if (raw[8] != PROTO_VER)
    {
        return PKT_ERR_VER;
    }
    if (raw_len != ((size_t)PKT_MIN_BIN_LEN + (size_t)plen))
    {
        return PKT_ERR_LEN;
    }

    /* ---- CRC-16 integrity --------------------------------------- */
    crc_rx = Buf_GetU16(&raw[9U + (size_t)plen]);
    crc_ok = Crc16_Compute(raw, 9U + (size_t)plen);
    if (crc_rx != crc_ok)
    {
        return PKT_ERR_CRC;
    }

    /* ---- Populate output structure ------------------------------ */
    (void)memset(pkt, 0, sizeof(*pkt));
    pkt->dev_id      = Buf_GetU16(&raw[2]);
    pkt->type        = raw[4];
    pkt->cmd         = Buf_GetU16(&raw[5]);
    pkt->payload_len = plen;
    pkt->proto_ver   = raw[8];
    if (plen > 0U)
    {
        (void)memcpy(pkt->payload, &raw[9], (size_t)plen);
    }

    return PKT_ERR_OK;
}

/* ================================================================== */
/*  Packet_Encode  --  Packet_s  -->  ASCII-hex                        */
/* ================================================================== */

bool Packet_Encode(const Packet_s *pkt, char *out, size_t out_cap, size_t *written)
{
    uint8_t  raw[PKT_MAX_BIN_LEN];
    size_t   raw_len;
    uint16_t crc;

    if ((pkt == NULL) || (out == NULL))
    {
        return false;
    }

    raw_len = (size_t)PKT_MIN_BIN_LEN + (size_t)pkt->payload_len;
    if (out_cap < (raw_len * 2U))
    {
        return false;
    }

    /* ---- Build binary frame in raw[] ---------------------------- */
    Buf_SetU16(&raw[0], FRAME_SOF);
    Buf_SetU16(&raw[2], pkt->dev_id);
    raw[4] = pkt->type;
    Buf_SetU16(&raw[5], pkt->cmd);
    raw[7] = pkt->payload_len;
    raw[8] = PROTO_VER;
    if (pkt->payload_len > 0U)
    {
        (void)memcpy(&raw[9], pkt->payload, (size_t)pkt->payload_len);
    }

    /* ---- Append CRC and EOF marker ------------------------------ */
    crc = Crc16_Compute(raw, 9U + (size_t)pkt->payload_len);
    Buf_SetU16(&raw[9U  + (size_t)pkt->payload_len], crc);
    Buf_SetU16(&raw[11U + (size_t)pkt->payload_len], FRAME_EOF);

    /* ---- Convert to ASCII hex ---------------------------------- */
    if (!Bin_ToHexStr(raw, raw_len, out, out_cap))
    {
        return false;
    }
    if (written != NULL)
    {
        *written = raw_len * 2U;
    }

    return true;
}

/* ================================================================== */
/*  Response / error frame constructors                                */
/* ================================================================== */

bool Packet_MakeResp(uint16_t       dev_id,
                     uint16_t       cmd,
                     const uint8_t  *payload,
                     uint8_t        plen,
                     Packet_s       *pkt)
{
    if ((pkt == NULL) || ((plen > 0U) && (payload == NULL)))
    {
        return false;
    }

    (void)memset(pkt, 0, sizeof(*pkt));
    pkt->dev_id      = dev_id;
    pkt->type        = PKT_TYPE_RESP;
    pkt->cmd         = cmd;
    pkt->payload_len = plen;
    pkt->proto_ver   = PROTO_VER;
    if (plen > 0U)
    {
        (void)memcpy(pkt->payload, payload, (size_t)plen);
    }

    return true;
}

bool Packet_MakeOk(uint16_t dev_id, uint16_t cmd, Packet_s *pkt)
{
    static const uint8_t kOkByte = 0xFFU;

    return Packet_MakeResp(dev_id, cmd, &kOkByte, 1U, pkt);
}

bool Packet_MakeErr(uint16_t       dev_id,
                    uint16_t       cmd,
                    const uint8_t  *payload,
                    uint8_t        plen,
                    Packet_s       *pkt)
{
    if ((pkt == NULL) || ((plen > 0U) && (payload == NULL)))
    {
        return false;
    }

    (void)memset(pkt, 0, sizeof(*pkt));
    pkt->dev_id      = dev_id;
    pkt->type        = PKT_TYPE_ERR;
    pkt->cmd         = cmd;
    pkt->payload_len = plen;
    pkt->proto_ver   = PROTO_VER;
    if (plen > 0U)
    {
        (void)memcpy(pkt->payload, payload, (size_t)plen);
    }

    return true;
}
