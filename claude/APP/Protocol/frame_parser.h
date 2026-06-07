#ifndef __FRAME_PARSER_H
#define __FRAME_PARSER_H
#include "main.h"
#define FRAME_BUF_SIZE 512
void frame_parser_feed(uint8_t byte);
void frame_parser_process(void);
extern volatile uint8_t g_frame_ready;
extern uint8_t g_frame_buf[FRAME_BUF_SIZE];
extern uint16_t g_frame_len;
#endif
