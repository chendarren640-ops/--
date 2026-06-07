#ifndef __HEADERFILES_H
#define __HEADERFILES_H

#include "gd32f4xx_libopt.h"
#include "gd32f4xx.h"


#include "systick.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Driver */
#include "bsp_flash.h"
#include "bsp_usart_rs485.h"

/* Protocol */
#include "protocol_crc.h"
#include "protocol_ascii_hex.h"
#include "boot_protocol.h"

/* Function */
#include "boot_jump.h"
#include "boot_flag.h"
#include "boot_update.h"

/* OLED */
#include "OLED.h"

#endif

/****************************End*****************************/
