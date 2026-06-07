/* SPDX-License-Identifier: MIT */

/**
 * @file    ota_core.h
 * @brief   OTA firmware upgrade receiver — state machine API.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief   Reset the OTA receiver state machine and ring buffer.
 *
 * Clears parsing state and the software FIFO but does NOT erase
 * already-written flash contents in the download cache.
 */
void OtaCore_Reset(void);

/**
 * @brief   Feed raw serial bytes into the OTA receiver.
 *
 * Called from the UART DMA polling layer. Data is buffered into the
 * software FIFO for later consumption by OtaCore_Poll().
 *
 * @param   data  Pointer to newly received bytes
 * @param   len   Number of bytes in @p data
 */
void OtaCore_FeedRaw(const uint8_t *data, uint32_t len);

/**
 * @brief   Execute the OTA receive state machine.
 *
 * Searches for OtaStreamHdr_s, validates it, erases the download cache,
 * writes the raw App.bin payload to flash, verifies the whole-image CRC,
 * commits BootLoader parameters, and resets into the BootLoader to
 * complete installation.
 */
void OtaCore_Poll(void);

/**
 * @brief   Request a fast bootloader upgrade (no payload — just flag).
 * @return  1 on success, 0 on failure
 */
uint8_t OtaCore_ReqUpgrade(void);

/**
 * @brief   Save the current APP communication parameters to the BootLoader
 *          parameter page.
 *
 * Required so the BootLoader uses the correct device ID and baud rate:
 *   - M-01 switches the host to 115200; BL must follow or it will miss
 *     0x0502/0x0503.
 *   - L-01 changes the device ID; BL must match or N-02/N-03 will fail.
 *
 * Skips the flash write if the parameters are already current.
 *
 * @param   device_id  Current device ID (0x0001..0xFFFE)
 * @param   baud_code  Baud rate map code: 0x11/0x12/0x13/0x14
 * @return  1 on success, 0 on failure
 */
uint8_t OtaCore_SaveCommParams(uint16_t device_id, uint8_t baud_code);

#ifdef __cplusplus
}
#endif
