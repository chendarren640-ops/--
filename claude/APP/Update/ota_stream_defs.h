/* SPDX-License-Identifier: MIT */

/**
 * @file    ota_stream_defs.h
 * @brief   OTA stream header format — the packet that prefixes the raw
 *          App.bin payload during serial firmware upgrade.
 *
 * Full OTA file layout:
 *   [OtaStreamHdr_s][raw App.bin]
 *
 * The App-side receiver parses this header, validates it, then writes
 * the following binary data into the download cache area. The header
 * itself is NOT written to flash, so the BootLoader still sees a valid
 * vector table at the cache base address.
 */

#pragma once

#include <stdint.h>

#if defined(__CC_ARM)
#define OTA_PACKED_STRUCT_BEGIN     __packed struct
#define OTA_PACKED_STRUCT_END
#else
#define OTA_PACKED_STRUCT_BEGIN     struct
#define OTA_PACKED_STRUCT_END       __attribute__((packed))
#endif

#define OTA_STREAM_MAGIC            0x12345678UL   /**< Must match OTA_PACKAGE_MAGIC */
#define OTA_STREAM_HDR_VER          3U             /**< Header protocol version */
#define OTA_IMG_APP1                1U             /**< Image type: APP1 */

/**
 * @brief   Serial OTA stream header (packed, little-endian).
 *
 * The header CRC covers all fields up to (but not including) @p header_crc32.
 */
typedef OTA_PACKED_STRUCT_BEGIN
{
    uint32_t magic;          /**< Must equal OTA_STREAM_MAGIC */
    uint16_t header_size;    /**< Must equal sizeof(OtaStreamHdr_s) */
    uint16_t header_version; /**< Must equal OTA_STREAM_HDR_VER */
    uint32_t app_version;    /**< Application version carried by this OTA */
    uint32_t app_size;       /**< Raw App.bin size in bytes */
    uint32_t app_crc32;      /**< CRC32 of the complete raw App.bin */
    uint32_t target_addr;    /**< Must match APP1_BASE */
    uint32_t image_type;     /**< Must equal OTA_IMG_APP1 */
    uint32_t header_crc32;   /**< CRC32 of all preceding header fields */
} OTA_PACKED_STRUCT_END OtaStreamHdr_s;
