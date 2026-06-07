/* SPDX-License-Identifier: MIT */

/**
 * @file    ring_fifo.h
 * @brief   Circular FIFO buffer declarations
 *
 * Lightweight FIFO backed by a caller-owned byte array. One byte of
 * the total capacity is reserved internally to distinguish the full
 * vs. empty state. All functions are re-entrant but not ISR-safe
 * unless externally protected.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Circular FIFO control structure
 *
 * The caller owns the memory pointed to by buffer and must ensure
 * its lifetime covers the use of the FIFO.
 */
typedef struct
{
    /** @brief Externally-provided data buffer */
    uint8_t *buffer;
    /** @brief Total buffer size in bytes (usable capacity = size - 1) */
    uint32_t size;
    /** @brief Write index (points to the next write slot) */
    uint32_t head;
    /** @brief Read index (points to the next read slot) */
    uint32_t tail;
} Fifo_s;

/**
 * @brief  Initialize a FIFO by binding it to an external buffer
 *
 * @param[in,out] fifo    FIFO control structure
 * @param[in]     buffer  externally-provided data buffer
 * @param[in]     size    buffer size in bytes
 */
void Fifo_Init(Fifo_s *fifo, uint8_t *buffer, uint32_t size);

/**
 * @brief  Return the number of bytes available for reading
 *
 * @param[in] fifo  FIFO control structure
 * @return          available byte count
 */
uint32_t Fifo_Avail(const Fifo_s *fifo);

/**
 * @brief  Return the number of free bytes remaining in the FIFO
 *
 * One byte is reserved to distinguish full vs. empty, so the
 * maximum writable count is size - 1.
 *
 * @param[in] fifo  FIFO control structure
 * @return          free byte count
 */
uint32_t Fifo_Free(const Fifo_s *fifo);

/**
 * @brief  Write data into the FIFO
 *
 * If free space is insufficient no bytes are written and false
 * is returned (atomic write).
 *
 * @param[in,out] fifo  FIFO control structure
 * @param[in]     data  source data buffer
 * @param[in]     len   number of bytes to write
 * @return              true on success, false on full or invalid
 */
bool Fifo_Put(Fifo_s *fifo, const uint8_t *data, uint32_t len);

/**
 * @brief  Peek at data without advancing the read pointer
 *
 * @param[in]  fifo  FIFO control structure
 * @param[out] data  destination buffer
 * @param[in]  len   maximum number of bytes to peek
 * @return           actual number of bytes copied
 */
uint32_t Fifo_Peek(const Fifo_s *fifo, uint8_t *data, uint32_t len);

/**
 * @brief  Read data from the FIFO and advance the read pointer
 *
 * @param[in,out] fifo  FIFO control structure
 * @param[out]    data  destination buffer
 * @param[in]     len   maximum number of bytes to read
 * @return              actual number of bytes read
 */
uint32_t Fifo_Get(Fifo_s *fifo, uint8_t *data, uint32_t len);

/**
 * @brief  Discard bytes without copying them to a caller buffer
 *
 * Useful for skipping invalid bytes or already-processed headers
 * during protocol parsing.
 *
 * @param[in,out] fifo  FIFO control structure
 * @param[in]     len   number of bytes to drop
 */
void Fifo_Drop(Fifo_s *fifo, uint32_t len);

/**
 * @brief  Clear the FIFO by resetting head and tail
 *
 * Does not zero the underlying buffer content.
 *
 * @param[in,out] fifo  FIFO control structure
 */
void Fifo_Clear(Fifo_s *fifo);
