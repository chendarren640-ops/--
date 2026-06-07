/* SPDX-License-Identifier: MIT */

/**
 * @file    lib_select.h
 * @brief   GD32F4xx peripheral library selection
 *
 * Selectively includes the GD32F4xx firmware library headers required
 * by the project. Guards against accidental inclusion on unsupported
 * MCU variants.
 */

#pragma once

#if defined (GD32F450) || defined (GD32F405) || defined (GD32F407) || \
    defined (GD32F470) || defined (GD32F425) || defined (GD32F427)
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_fmc.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_pmu.h"
#include "gd32f4xx_usart.h"
#endif
