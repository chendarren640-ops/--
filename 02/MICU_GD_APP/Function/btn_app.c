/* SPDX-License-Identifier: MIT */

/**
 * @file    btn_app.c
 * @brief   Application-layer button driver using the embedded-button (ebtn)
 *          library.
 *
 * Manages up to seven physical buttons with debounce, click, double-click,
 * and long-press detection provided by the ebtn library.
 */

#include "board_defs.h"

/**
 * @brief Logical identifiers for the seven user buttons.
 */
typedef enum
{
    BTN_ID_0 = 0,
    BTN_ID_1,
    BTN_ID_2,
    BTN_ID_3,
    BTN_ID_4,
    BTN_ID_5,
    BTN_ID_6,
    BTN_ID_MAX,
} BtnId_e;

/** Array of ebtn button objects, one per physical button. */
static ebtn_button_t s_Buttons[BTN_ID_MAX];

/**
 * @brief Read the raw pin state for a given button.
 * @param  btn  Pointer to the ebtn button object.
 * @return      1 if the button is pressed (active low logic), 0 otherwise.
 */
static uint8_t Btn_ReadPin(ebtn_button_t *btn)
{
    switch (btn->id)
    {
    case BTN_ID_0:
        return !KEY1_READ;
    case BTN_ID_1:
        return !KEY2_READ;
    case BTN_ID_2:
        return !KEY3_READ;
    case BTN_ID_3:
        return !KEY4_READ;
    case BTN_ID_4:
        return !KEY5_READ;
    case BTN_ID_5:
        return !KEY6_READ;
    case BTN_ID_6:
        return !KEYW_READ;
    default:
        return 0;
    }
}

/**
 * @brief Handle a single-click event for a given button.
 * @param btn  Pointer to the ebtn button object.
 */
static void Btn_OnClick(ebtn_button_t *btn)
{
    switch (btn->id)
    {
    case BTN_ID_0:
        break;
    case BTN_ID_1:
        break;
    case BTN_ID_2:
        break;
    case BTN_ID_3:
        break;
    case BTN_ID_4:
        break;
    case BTN_ID_5:
        break;
    case BTN_ID_6:
        break;
    default:
        break;
    }
}

/**
 * @brief Dispatch a button state event to the appropriate handler.
 * @param btn  Pointer to the ebtn button object.
 */
static void Btn_OnEvent(ebtn_button_t *btn)
{
    switch (btn->btn_state)
    {
    case EBTN_STATE_CLICK:
        Btn_OnClick(btn);
        break;

    case EBTN_STATE_DOUBLE_CLICK:
        break;

    case EBTN_STATE_LONG_PRESS:
        break;

    default:
        break;
    }
}

/**
 * @brief Initialise all user buttons with the ebtn library.
 */
void Btn_Init(void)
{
    uint8_t i;

    for (i = 0; i < BTN_ID_MAX; i++)
    {
        ebtn_button_init(&s_Buttons[i], i, 1, Btn_ReadPin, Btn_OnEvent);
    }
}

/**
 * @brief Button polling task, invoked every 5 ms from the scheduler.
 */
void Btn_Process(void)
{
    ebtn_button_process_all(s_Buttons, BTN_ID_MAX);
}
