#ifndef __BOOT_JUMP_H
#define __BOOT_JUMP_H

#include "gd32f4xx.h"
#include <stdint.h>
#include "bsp_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*iap_function_t)(void);

uint8_t BootJump_IsAppValid(void);
void BootJump_JumpToApp(void);

#ifdef __cplusplus
}
#endif

#endif

