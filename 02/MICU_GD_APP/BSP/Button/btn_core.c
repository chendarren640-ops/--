/* SPDX-License-Identifier: MIT */

/**
 * @file    btn_core.c
 * @brief   Button debouncer implementation.
 *
 * Internal state machine:
 *  - EBTN_SM_IDLE             (0)  Waiting for first press
 *  - EBTN_SM_PRESSED          (1)  Button held, counting ticks
 *  - EBTN_SM_RELEASED_WAIT    (2)  Waiting for possible double-click
 *  - EBTN_SM_LONG_WAIT_RELEASE (3) Waiting for release after long-press
 */

#include "btn_core.h"

#define EBTN_SM_IDLE              0U
#define EBTN_SM_PRESSED           1U
#define EBTN_SM_RELEASED_WAIT     2U
#define EBTN_SM_LONG_WAIT_RELEASE 3U

/**
 * @brief   Initialise a button instance.
 */
void Ebtn_Init(EbtnBtn_s *btn, uint8_t id, uint8_t active_level,
               EbtnReadFn_t read, EbtnHandlerFn_t handler)
{
    if (btn == 0)
    {
        return;
    }

    btn->ticks        = 0;
    btn->id           = id;
    btn->state        = EBTN_SM_IDLE;
    btn->repeat       = 0;
    btn->debounce_cnt = 0;
    btn->active_level = active_level;
    btn->btn_state    = EBTN_ST_IDLE;
    btn->read         = read;
    btn->handler      = handler;
    btn->level        = (btn->read != 0) ? btn->read(btn) : (uint8_t)!active_level;
}

/**
 * @brief   Scan and process a single button.
 */
void Ebtn_Process(EbtnBtn_s *btn)
{
    uint8_t gpio_level;

    if ((btn == 0) || (btn->read == 0))
    {
        return;
    }

    gpio_level = btn->read(btn);

    if (btn->state > EBTN_SM_IDLE)
    {
        btn->ticks++;
    }

    if (btn->level != gpio_level)
    {
        if (++btn->debounce_cnt >= EBTN_DB_TICKS)
        {
            btn->level = gpio_level;
            btn->debounce_cnt = 0;
        }
    }
    else
    {
        btn->debounce_cnt = 0;
    }

    switch (btn->state)
    {
    case EBTN_SM_IDLE:
        if (btn->level == btn->active_level)
        {
            btn->state  = EBTN_SM_PRESSED;
            btn->ticks  = 0;
            btn->repeat = 1;
        }
        else
        {
            btn->btn_state = EBTN_ST_IDLE;
        }
        break;

    case EBTN_SM_PRESSED:
        if (btn->level != btn->active_level)
        {
            btn->ticks = 0;
            btn->state = EBTN_SM_RELEASED_WAIT;
        }
        else if (btn->ticks >= EBTN_LP_TICKS)
        {
            btn->btn_state = EBTN_ST_LONG_PRESS;
            if (btn->handler != 0)
            {
                btn->handler(btn);
            }
            btn->ticks  = 0;
            btn->repeat = 0;
            btn->state  = EBTN_SM_LONG_WAIT_RELEASE;
        }
        break;

    case EBTN_SM_RELEASED_WAIT:
        if (btn->ticks > EBTN_DC_TICKS)
        {
            if (btn->repeat == 1)
            {
                btn->btn_state = EBTN_ST_CLICK;
                if (btn->handler != 0)
                {
                    btn->handler(btn);
                }
            }
            else if (btn->repeat == 2)
            {
                btn->btn_state = EBTN_ST_DOUBLE_CLICK;
                if (btn->handler != 0)
                {
                    btn->handler(btn);
                }
            }

            btn->state  = EBTN_SM_IDLE;
            btn->repeat = 0;
        }
        else if (btn->level == btn->active_level)
        {
            if (btn->repeat < 2)
            {
                btn->repeat++;
            }
            btn->state = EBTN_SM_PRESSED;
            btn->ticks = 0;
        }
        break;

    case EBTN_SM_LONG_WAIT_RELEASE:
        if (btn->level != btn->active_level)
        {
            btn->state  = EBTN_SM_IDLE;
            btn->repeat = 0;
            btn->ticks  = 0;
        }
        break;

    default:
        btn->state        = EBTN_SM_IDLE;
        btn->repeat       = 0;
        btn->ticks        = 0;
        btn->debounce_cnt = 0;
        break;
    }
}

/**
 * @brief   Scan and process an array of buttons.
 */
void Ebtn_ProcessAll(EbtnBtn_s *btns, uint8_t count)
{
    uint8_t i;

    if (btns == 0)
    {
        return;
    }

    for (i = 0; i < count; i++)
    {
        Ebtn_Process(&btns[i]);
    }
}
