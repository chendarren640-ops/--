/* SPDX-License-Identifier: MIT */

/**
 * @file    btn_core.h
 * @brief   Hardware-agnostic button debouncer with click / double-click / long-press
 *          detection.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define EBTN_ARR_CNT(_arr)  (sizeof(_arr) / sizeof((_arr)[0]))

#ifndef EBTN_DB_TICKS
#define EBTN_DB_TICKS        3U    /**< Debounce sample count */
#endif

#ifndef EBTN_LP_TICKS
#define EBTN_LP_TICKS        50U   /**< Long-press threshold in ticks */
#endif

#ifndef EBTN_DC_TICKS
#define EBTN_DC_TICKS        5U    /**< Double-click gap window in ticks */
#endif

/** Button event reported to the handler */
typedef enum
{
    EBTN_ST_IDLE         = 0,
    EBTN_ST_LONG_PRESS,
    EBTN_ST_CLICK,
    EBTN_ST_DOUBLE_CLICK,
} EbtnSt_e;

struct EbtnBtn_s;

typedef uint8_t (*EbtnReadFn_t)(struct EbtnBtn_s *btn);
typedef void    (*EbtnHandlerFn_t)(struct EbtnBtn_s *btn);

/** Button instance descriptor */
typedef struct EbtnBtn_s
{
    uint16_t         ticks;
    uint8_t          level;
    uint8_t          id;
    uint8_t          state;
    uint8_t          repeat;
    uint8_t          debounce_cnt;
    uint8_t          active_level;
    EbtnSt_e         btn_state;
    EbtnReadFn_t     read;
    EbtnHandlerFn_t  handler;
} EbtnBtn_s;

/**
 * @brief   Initialise a button instance.
 * @param   btn           Pointer to button descriptor
 * @param   id            User-defined button ID
 * @param   active_level  Logic level when pressed (0 or 1)
 * @param   read          Callback to read the current GPIO level
 * @param   handler       Callback invoked on click / double-click / long-press
 */
void Ebtn_Init(EbtnBtn_s *btn, uint8_t id, uint8_t active_level,
               EbtnReadFn_t read, EbtnHandlerFn_t handler);

/**
 * @brief   Scan and process a single button.
 * @param   btn  Pointer to button descriptor
 */
void Ebtn_Process(EbtnBtn_s *btn);

/**
 * @brief   Scan and process an array of buttons.
 * @param   btns   Pointer to button descriptor array
 * @param   count  Number of buttons in the array
 */
void Ebtn_ProcessAll(EbtnBtn_s *btns, uint8_t count);

#ifdef __cplusplus
}
#endif
