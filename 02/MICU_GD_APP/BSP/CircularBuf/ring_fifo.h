/* SPDX-License-Identifier: MIT */

/**
 * @file    ring_fifo.h
 * @brief   Lightweight ring-buffer / FIFO implementation.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief   Ring-buffer control block.
 */
typedef struct
{
    uint8_t *buffer;  /**< Externally-provided data buffer */
    uint32_t size;    /**< Total buffer size in bytes; usable capacity = size - 1 */
    uint32_t head;    /**< Write index (next insertion position) */
    uint32_t tail;    /**< Read index (next extraction position) */
} Fifo_s;

/**
 * @brief   Initialise a FIFO.
 * @param   fifo    FIFO control block
 * @param   buffer  Externally-provided storage
 * @param   size    Buffer size in bytes
 */
void Fifo_Init(Fifo_s *fifo, uint8_t *buffer, uint32_t size);

/**
 * @brief   Get the number of readable bytes.
 * @param   fifo  FIFO control block
 * @return  Number of bytes available for reading
 */
uint32_t Fifo_Avail(const Fifo_s *fifo);

/**
 * @brief   Get the remaining writable capacity.
 * @param   fifo  FIFO control block
 * @return  Free space in bytes (always >= 1 reserved for empty/full discrimination)
 */
uint32_t Fifo_Free(const Fifo_s *fifo);

/**
 * @brief   Write data into the FIFO (all-or-nothing).
 * @param   fifo  FIFO control block
 * @param   data  Source data pointer
 * @param   len   Number of bytes to write
 * @return  true on success, false if insufficient space or invalid parameters
 */
bool Fifo_Put(Fifo_s *fifo, const uint8_t *data, uint32_t len);

/**
 * @brief   Peek at FIFO data without advancing the read pointer.
 * @param   fifo  FIFO control block
 * @param   data  Destination buffer
 * @param   len   Requested byte count
 * @return  Actual number of bytes copied
 */
uint32_t Fifo_Peek(const Fifo_s *fifo, uint8_t *data, uint32_t len);

/**
 * @brief   Read data from the FIFO and advance the read pointer.
 * @param   fifo  FIFO control block
 * @param   data  Destination buffer
 * @param   len   Requested byte count
 * @return  Actual number of bytes copied
 */
uint32_t Fifo_Get(Fifo_s *fifo, uint8_t *data, uint32_t len);

/**
 * @brief   Discard bytes from the FIFO without reading.
 * @param   fifo  FIFO control block
 * @param   len   Number of bytes to drop
 */
void Fifo_Drop(Fifo_s *fifo, uint32_t len);

/**
 * @brief   Clear the FIFO (reset read/write pointers only).
 * @param   fifo  FIFO control block
 */
void Fifo_Clear(Fifo_s *fifo);
