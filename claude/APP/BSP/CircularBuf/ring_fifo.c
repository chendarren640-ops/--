/* SPDX-License-Identifier: MIT */

/**
 * @file    ring_fifo.c
 * @brief   Ring-buffer / FIFO implementation.
 */

#include "ring_fifo.h"
#include <stddef.h>

/**
 * @brief   Initialise a FIFO.
 */
void Fifo_Init(Fifo_s *fifo, uint8_t *buffer, uint32_t size)
{
    if (fifo == NULL)
    {
        return;
    }

    fifo->buffer = buffer;
    fifo->size   = size;
    fifo->head   = 0U;
    fifo->tail   = 0U;
}

/**
 * @brief   Get the number of readable bytes.
 */
uint32_t Fifo_Avail(const Fifo_s *fifo)
{
    if ((fifo == NULL) || (fifo->buffer == NULL) || (fifo->size == 0U))
    {
        return 0U;
    }

    if (fifo->head >= fifo->tail)
    {
        return fifo->head - fifo->tail;
    }

    return fifo->size - fifo->tail + fifo->head;
}

/**
 * @brief   Get the remaining writable capacity.
 */
uint32_t Fifo_Free(const Fifo_s *fifo)
{
    if ((fifo == NULL) || (fifo->buffer == NULL) || (fifo->size == 0U))
    {
        return 0U;
    }

    return fifo->size - Fifo_Avail(fifo) - 1U;
}

/**
 * @brief   Write data into the FIFO (all-or-nothing).
 */
bool Fifo_Put(Fifo_s *fifo, const uint8_t *data, uint32_t len)
{
    uint32_t i;

    if ((fifo == NULL) || (data == NULL))
    {
        return false;
    }
    if (Fifo_Free(fifo) < len)
    {
        return false;
    }

    for (i = 0U; i < len; i++)
    {
        fifo->buffer[fifo->head] = data[i];
        fifo->head = (fifo->head + 1U) % fifo->size;
    }

    return true;
}

/**
 * @brief   Peek at FIFO data without advancing the read pointer.
 */
uint32_t Fifo_Peek(const Fifo_s *fifo, uint8_t *data, uint32_t len)
{
    uint32_t available;
    uint32_t to_read;
    uint32_t tail;
    uint32_t i;

    if ((fifo == NULL) || (data == NULL) || (fifo->buffer == NULL) || (fifo->size == 0U))
    {
        return 0U;
    }

    available = Fifo_Avail(fifo);
    to_read   = (len < available) ? len : available;
    tail      = fifo->tail;

    for (i = 0U; i < to_read; i++)
    {
        data[i] = fifo->buffer[tail];
        tail = (tail + 1U) % fifo->size;
    }

    return to_read;
}

/**
 * @brief   Read data from the FIFO and advance the read pointer.
 */
uint32_t Fifo_Get(Fifo_s *fifo, uint8_t *data, uint32_t len)
{
    uint32_t available;
    uint32_t to_read;
    uint32_t i;

    if ((fifo == NULL) || (data == NULL) || (fifo->buffer == NULL) || (fifo->size == 0U))
    {
        return 0U;
    }

    available = Fifo_Avail(fifo);
    to_read   = (len < available) ? len : available;

    for (i = 0U; i < to_read; i++)
    {
        data[i] = fifo->buffer[fifo->tail];
        fifo->tail = (fifo->tail + 1U) % fifo->size;
    }

    return to_read;
}

/**
 * @brief   Discard bytes from the FIFO without reading.
 */
void Fifo_Drop(Fifo_s *fifo, uint32_t len)
{
    uint32_t available;
    uint32_t to_drop;

    if ((fifo == NULL) || (fifo->buffer == NULL) || (fifo->size == 0U))
    {
        return;
    }

    available = Fifo_Avail(fifo);
    to_drop   = (len < available) ? len : available;
    fifo->tail = (fifo->tail + to_drop) % fifo->size;
}

/**
 * @brief   Clear the FIFO (reset read/write pointers only).
 */
void Fifo_Clear(Fifo_s *fifo)
{
    if (fifo == NULL)
    {
        return;
    }

    fifo->head = 0U;
    fifo->tail = 0U;
}
