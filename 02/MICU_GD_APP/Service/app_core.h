/* SPDX-License-Identifier: MIT */

/**
 * @file app_core.h
 * @brief Master header for the application core module.
 *
 * Declares the public API that ties together parameter persistence,
 * command dispatch, auto-reporting, alarm management, and heartbeat
 * transmission.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ------------------------------------------------------------------ */
/** @name External sensor data (defined by ADC / PT100 driver layers)  */
/* ------------------------------------------------------------------ */
extern uint16_t g_AdcRaw[2];
extern float    g_Pt100Temp;

/* ------------------------------------------------------------------ */
/** @name Application lifecycle                                          */
/* ------------------------------------------------------------------ */

void AppCtrl_Init(void);
void AppCtrl_Poll(void);
void AppCtrl_OnFrame(const char *ascii, size_t len);

/* ------------------------------------------------------------------ */
/** @name Device identity helpers                                        */
/* ------------------------------------------------------------------ */

uint16_t AppCtrl_GetDevId(void);
uint8_t  AppCtrl_IsAutoRpt(void);

#define STARTUP_BEAT_COUNT  1U

/* ------------------------------------------------------------------ */
/** @name Heartbeat                                                      */
/* ------------------------------------------------------------------ */

void Heartbeat_Send(void);

/* ------------------------------------------------------------------ */
/** @name PHY-level frame / text transmission                            */
/* ------------------------------------------------------------------ */

void Frame_Tx(const void *frame);
void Text_Tx(const char *text);

/* ------------------------------------------------------------------ */
/** @name Wire-format helpers (big-endian encode / decode)                */
/* ------------------------------------------------------------------ */

void     Wire_PutU16BE(uint8_t *out, uint16_t value);
void     Wire_PutU32BE(uint8_t *out, uint32_t value);
uint32_t Wire_GetU32BE(const uint8_t *in);
void     Wire_PutFloatBE(uint8_t *out, float value);
float    Wire_GetFloatBE(const uint8_t *in);

/* ------------------------------------------------------------------ */
/** @name Parameter store accessors                                      */
/* ------------------------------------------------------------------ */

void     Param_Load(void);
void     Param_Save(void);
uint16_t Param_GetDevId(void);
void     Param_SetDevId(uint16_t id);
uint8_t  Param_GetBaudCode(void);
void     Param_SetBaudCode(uint8_t code);
float    Param_GetChScale(uint8_t channel);
void     Param_SetChScale(uint8_t channel, float scale);
float    Param_GetThreshold(uint8_t channel);
void     Param_SetThreshold(uint8_t channel, float threshold);
uint16_t Param_GetDacValue(void);
void     Param_SetDacValue(uint16_t value);
uint32_t Baud_FromCode(uint8_t code);

/* ------------------------------------------------------------------ */
/** @name Runtime state accessors                                        */
/* ------------------------------------------------------------------ */

uint32_t Time_GetUtc(void);
void     Time_SetUtc(uint32_t utc);
float    Ch_ReadValue(uint8_t channel);
void     Rpt_BuildPayload(uint8_t *payload);

uint8_t  AutoRpt_GetEnabled(void);
void     AutoRpt_SetEnabled(uint8_t enabled);
uint32_t AutoRpt_GetInterval(void);
void     AutoRpt_SetInterval(uint32_t ms);
uint32_t AutoRpt_GetLastMs(void);
void     AutoRpt_SetLastMs(uint32_t ms);

uint8_t  Alarm_GetActiveReport(void);
void     Alarm_SetActiveReport(uint8_t active);
void     Alarm_ResetOverThreshold(uint8_t channel);
void     Alarm_SetOverThreshold(uint8_t channel, uint8_t val);
uint8_t  Alarm_GetOverThreshold(uint8_t channel);
void     Alarm_Check(uint8_t channel);
void     Alarm_Query(void);
void     Alarm_Clear(void);
void     Alarm_Reset(void);

uint8_t  Beat_GetSent(void);
void     Beat_SetSent(uint8_t count);
uint32_t Beat_GetLastMs(void);
void     Beat_SetLastMs(uint32_t ms);

/* ------------------------------------------------------------------ */
/** @name Command dispatch                                               */
/* ------------------------------------------------------------------ */

void Cmd_Dispatch(const void *request);

#ifdef __cplusplus
}
#endif
