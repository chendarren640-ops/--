/* SPDX-License-Identifier: MIT */

/**
 * @file    ring_fifo.c
 * @brief   Circular FIFO buffer implementation
 *
 * Implements a lightweight circular FIFO with external storage.
 * Head and tail pointers track the write and read positions within
 * the caller-provided buffer. One byte is reserved to distinguish
 * the full vs. empty state.
 */

#include "ring_fifo.h"
#include <stddef.h>

/**
 * @brief  Initialize a FIFO by binding it to an external buffer
 *
 * @param[in,out] fifo    FIFO control structure
 * @param[in]     buffer  externally-provided data buffer
 * @param[in]     size    buffer size in bytes
 */
void Fifo_Init(Fifo_s *fifo, uint8_t *buffer, uint32_t size)
{
    if(fifo == NULL)
    {
        return;
    }

    fifo->buffer = buffer;
    fifo->size   = size;
    fifo->head   = 0U;
    fifo->tail   = 0U;
}

/**
 * @brief  Return the number of bytes available for reading
 *
 * @param[in] fifo  FIFO control structure
 * @return          available byte count (0 if fifo is invalid)
 */
uint32_t Fifo_Avail(const Fifo_s *fifo)
{
    if((fifo == NULL) || (fifo->buffer == NULL) || (fifo->size == 0U))
    {
        return 0U;
    }

    if(fifo->head >= fifo->tail)
    {
        return fifo->head - fifo->tail;
    }

    return fifo->size - fifo->tail + fifo->head;
}

/**
 * @brief  Return the number of free bytes remaining in the FIFO
 *
 * The FIFO reserves one byte, so the maximum writable count is size - 1.
 *
 * @param[in] fifo  FIFO control structure
 * @return          free byte count (0 if fifo is invalid)
 */
uint32_t Fifo_Free(const Fifo_s *fifo)
{
    if((fifo == NULL) || (fifo->buffer == NULL) || (fifo->size == 0U))
    {
        return 0U;
    }

    return fifo->size - Fifo_Avail(fifo) - 1U;
}

/**
 * @brief  Write data into the FIFO
 *
 * The write is atomic — if the FIFO has insufficient free space
 * no bytes are written and false is returned.
 *
 * @param[in,out] fifo  FIFO control structure
 * @param[in]     data  source data buffer
 * @param[in]     len   number of bytes to write
 * @return              true on success, false if fifo is full or invalid
 */
bool Fifo_Put(Fifo_s *fifo, const uint8_t *data, uint32_t len)
{
    uint32_t i;

    if((fifo == NULL) || (data == NULL))
    {
        return false;
    }
    if(Fifo_Free(fifo) < len)
    {
        return false;
    }

    for(i = 0U; i < len; i++)
    {
        fifo->buffer[fifo->head] = data[i];
        fifo->head = (fifo->head + 1U) % fifo->size;
    }

    return true;
}

/**
 * @brief  Peek at data in the FIFO without advancing the read pointer
 *
 * Copies up to @p len bytes into @p data but does not move the tail.
 *
 * @param[in]  fifo  FIFO control structure
 * @param[out] data  destination buffer
 * @param[in]  len   maximum number of bytes to peek
 * @return           actual number of bytes copied
 */
uint32_t Fifo_Peek(const Fifo_s *fifo, uint8_t *data, uint32_t len)
{
    uint32_t available;
    uint32_t to_read;
    uint32_t tail;
    uint32_t i;

    if((fifo == NULL) || (data == NULL) || (fifo->buffer == NULL) || (fifo->size == 0U))
    {
        return 0U;
    }

    available = Fifo_Avail(fifo);
    to_read   = (len < available) ? len : available;
    tail      = fifo->tail;

    for(i = 0U; i < to_read; i++)
    {
        data[i] = fifo->buffer[tail];
        tail = (tail + 1U) % fifo->size;
    }

    return to_read;
}

/**
 * @brief  Read data from the FIFO and advance the read pointer
 *
 * Copies up to @p len bytes into @p data and moves the tail forward.
 *
 * @param[in,out] fifo  FIFO control structure
 * @param[out]    data  destination buffer
 * @param[in]     len   maximum number of bytes to read
 * @return              actual number of bytes read
 */
uint32_t Fifo_Get(Fifo_s *fifo, uint8_t *data, uint32_t len)
{
    uint32_t available;
    uint32_t to_read;
    uint32_t i;

    if((fifo == NULL) || (data == NULL) || (fifo->buffer == NULL) || (fifo->size == 0U))
    {
        return 0U;
    }

    available = Fifo_Avail(fifo);
    to_read   = (len < available) ? len : available;

    for(i = 0U; i < to_read; i++)
    {
        data[i] = fifo->buffer[fifo->tail];
        fifo->tail = (fifo->tail + 1U) % fifo->size;
    }

    return to_read;
}

/**
 * @brief  Discard bytes from the FIFO without copying them
 *
 * Advances the read pointer by up to @p len bytes, skipping data.
 *
 * @param[in,out] fifo  FIFO control structure
 * @param[in]     len   number of bytes to drop
 */
void Fifo_Drop(Fifo_s *fifo, uint32_t len)
{
    uint32_t available;
    uint32_t to_drop;

    if((fifo == NULL) || (fifo->buffer == NULL) || (fifo->size == 0U))
    {
        return;
    }

    available = Fifo_Avail(fifo);
    to_drop   = (len < available) ? len : available;
    fifo->tail = (fifo->tail + to_drop) % fifo->size;
}

/**
 * @brief  Clear the FIFO by resetting head and tail to zero
 *
 * The underlying buffer content is not zeroed; only the pointers
 * are reset.
 *
 * @param[in,out] fifo  FIFO control structure
 */
void Fifo_Clear(Fifo_s *fifo)
{
    if(fifo == NULL)
    {
        return;
    }

    fifo->head = 0U;
    fifo->tail = 0U;
}
