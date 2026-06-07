/* SPDX-License-Identifier: MIT */

/**
 * @file    packet_codec.h
 * @brief   Packet encoder / decoder -- converts between ASCII-hex wire
 *          representation and the internal @ref Packet_s structure.
 */

#pragma once

#include "protocol_defs.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief  Decode an ASCII-hex string into a Packet_s.
 * @param  ascii     Pointer to hex string received on the wire.
 * @param  ascii_len Number of hex characters.
 * @param  pkt       [out] Decoded packet structure.
 * @return @ref PacketErr_e status code.
 */
PacketErr_e Packet_Decode(const char *ascii, size_t ascii_len, Packet_s *pkt);

/**
 * @brief  Encode a Packet_s into an ASCII-hex string for transmission.
 * @param  pkt     Populated packet (all fields must be valid).
 * @param  out     Output buffer for hex characters.
 * @param  out_cap Capacity of @p out in characters.
 * @param  written [out, optional] Receives actual hex character count.
 * @return true on success, false on parameter error or buffer overflow.
 */
bool Packet_Encode(const Packet_s *pkt, char *out, size_t out_cap, size_t *written);

/**
 * @brief  Populate a response frame (type = PKT_TYPE_RESP).
 * @param  dev_id      Device identifier.
 * @param  cmd         Command being responded to.
 * @param  payload     Response data (may be NULL only if plen == 0).
 * @param  plen        Payload length in bytes.
 * @param  pkt         [out] Frame to populate.
 * @return true on success.
 */
bool Packet_MakeResp(uint16_t      dev_id,
                     uint16_t      cmd,
                     const uint8_t *payload,
                     uint8_t       plen,
                     Packet_s      *pkt);

/**
 * @brief  Convenience: build a single-byte "OK" response (payload = 0xFF).
 * @param  dev_id  Device identifier.
 * @param  cmd     Command being acknowledged.
 * @param  pkt     [out] Frame to populate.
 * @return true on success.
 */
bool Packet_MakeOk(uint16_t dev_id, uint16_t cmd, Packet_s *pkt);

/**
 * @brief  Populate an error frame (type = PKT_TYPE_ERR).
 * @param  dev_id  Device identifier.
 * @param  cmd     Associated command (or CMD_ERR_GENERIC).
 * @param  payload Error detail (may be NULL only if plen == 0).
 * @param  plen    Payload length in bytes.
 * @param  pkt     [out] Frame to populate.
 * @return true on success.
 */
bool Packet_MakeErr(uint16_t      dev_id,
                    uint16_t      cmd,
                    const uint8_t *payload,
                    uint8_t       plen,
                    Packet_s      *pkt);

#ifdef __cplusplus
}
#endif
