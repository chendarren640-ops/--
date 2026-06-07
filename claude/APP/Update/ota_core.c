/* SPDX-License-Identifier: MIT */

/**
 * @file    ota_core.c
 * @brief   OTA firmware upgrade receiver — state machine implementation.
 *
 * Architecture overview:
 *   1. UART DMA + IDLE interrupt feeds raw bytes into a software FIFO
 *      via OtaCore_FeedRaw().
 *   2. OtaCore_Poll() runs the state machine:
 *        - WAIT_HDR: scan the FIFO for OtaStreamHdr_s, validate header CRC
 *        - RECV:     erase download cache, write payload to flash, verify
 *                    whole-image CRC, commit BootLoader parameters, reset.
 *
 * Flash operations run from RAM (.ramfunc) because the internal flash
 * controller is not accessible while it is busy programming or erasing.
 */

#include "board_defs.h"
#include "ota_core.h"
#include "ring_fifo.h"
#include "flash_layout.h"
#include "flash_param.h"
#include "ota_stream_defs.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define OTA_FIFO_SZ            (32 * 1024U)
#define OTA_WRITE_CHUNK        512U

/** OTA receiver state */
typedef enum
{
    OTA_ST_WAIT_HDR = 0,
    OTA_ST_RECV
} OtaState_e;

/** OTA receiver context */
typedef struct
{
    OtaState_e state;
    uint32_t   app_version;
    uint32_t   expected_size;
    uint32_t   expected_crc32;
    uint32_t   recv_size;
    uint32_t   crc_acc;
    bool       rx_overflow;
} OtaCtx_s;

static OtaCtx_s  m_OtaCtx;
static uint8_t   m_ParamPageCache[FLASH_PARAM_PAGE_SZ];
static uint8_t   m_FifoStorage[OTA_FIFO_SZ];
static Fifo_s    m_Fifo;

/* --------------------------------------------------------------------------
 * CRC32 (Ethernet polynomial, reflected)
 * ------------------------------------------------------------------------ */

/**
 * @brief   Incrementally update a running CRC32 accumulator.
 * @param   crc   Current CRC value
 * @param   data  Input bytes
 * @param   len   Byte count
 * @return  Updated CRC value (not yet finalised)
 */
static uint32_t Crc32_Update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint32_t j;

    for (i = 0U; i < len; i++)
    {
        crc ^= data[i];
        for (j = 0U; j < 8U; j++)
        {
            crc = ((crc & 1U) != 0U) ? ((crc >> 1U) ^ 0xEDB88320UL) : (crc >> 1U);
        }
    }

    return crc;
}

/**
 * @brief   Finalise a CRC32 accumulator (invert all bits).
 * @param   crc  Running CRC value from Crc32_Update()
 * @return  Final CRC32
 */
static uint32_t Crc32_Final(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFUL;
}

/**
 * @brief   One-shot CRC32 calculation over a buffer.
 * @param   data  Input buffer
 * @param   len   Byte count
 * @return  CRC32 of the buffer
 */
static uint32_t Crc32_Calc(const uint8_t *data, uint32_t len)
{
    return Crc32_Final(Crc32_Update(0xFFFFFFFFUL, data, len));
}

/**
 * @brief   Calculate the BootParam_s CRC (excludes param_crc32 field itself).
 * @param   param  Pointer to BootParam_s
 * @return  CRC32 of all fields up to param_crc32
 */
static uint32_t Param_CalcCrc(const BootParam_s *param)
{
    return Crc32_Calc((const uint8_t *)param, (uint32_t)offsetof(BootParam_s, param_crc32));
}

/* --------------------------------------------------------------------------
 * Internal flash helpers (.ramfunc — run from RAM)
 * ------------------------------------------------------------------------ */

/**
 * @brief   Wait for the internal flash controller to become idle.
 * @return  true if ready within timeout, false on timeout
 */
__attribute__((section(".ramfunc"), noinline))
static bool Flash_WaitReady(void)
{
    uint32_t timeout = 0x3FFFFFUL;

    while ((RESET != fmc_flag_get(FMC_FLAG_BUSY)) && (timeout > 0U))
    {
        timeout--;
    }

    return (timeout > 0U);
}

/**
 * @brief   Clear flash controller status flags to avoid stale errors.
 */
__attribute__((section(".ramfunc"), noinline))
static void Flash_ClearFlags(void)
{
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
}

/**
 * @brief   Erase a range of internal flash pages.
 * @param   start_addr  First address to erase (page-aligned implicitly)
 * @param   size        Total byte count to erase
 * @return  true on success, false on failure
 */
__attribute__((section(".ramfunc"), noinline))
static bool Flash_ErasePages(uint32_t start_addr, uint32_t size)
{
    uint32_t end_addr;
    uint32_t page_addr;

    if ((size == 0U) || (start_addr < FLASH_BASE))
    {
        return false;
    }

    end_addr = start_addr + size - 1U;
    if (end_addr > FLASH_END)
    {
        return false;
    }

    page_addr = start_addr - (start_addr % FLASH_PAGE_SZ);
    end_addr  = end_addr - (end_addr % FLASH_PAGE_SZ);

    fmc_unlock();
    Flash_ClearFlags();

    while (page_addr <= end_addr)
    {
        fmc_page_erase(page_addr);
        if (!Flash_WaitReady())
        {
            fmc_lock();
            return false;
        }
        Flash_ClearFlags();
        page_addr += FLASH_PAGE_SZ;
    }

    fmc_lock();
    return true;
}

/**
 * @brief   Program bytes to internal flash (word-aligned where possible).
 * @param   addr  Destination flash address
 * @param   data  Source data
 * @param   size  Byte count
 * @return  true on success, false on failure
 */
__attribute__((section(".ramfunc"), noinline))
static bool Flash_ProgBytes(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint32_t word_value;

    if ((data == NULL) || (size == 0U))
    {
        return false;
    }

    if ((addr < FLASH_BASE) || ((addr + size - 1U) > FLASH_END))
    {
        return false;
    }

    fmc_unlock();
    Flash_ClearFlags();

    /* Word-aligned bulk write */
    while (((addr & 3UL) == 0UL) && (size >= 4U))
    {
        memcpy(&word_value, data, sizeof(word_value));
        fmc_word_program(addr, word_value);
        if (!Flash_WaitReady())
        {
            fmc_lock();
            return false;
        }
        addr += 4UL;
        data += 4U;
        size -= 4U;
    }

    /* Byte-level tail */
    for (i = 0U; i < size; i++)
    {
        fmc_byte_program(addr + i, data[i]);
        if (!Flash_WaitReady())
        {
            fmc_lock();
            return false;
        }
    }

    Flash_ClearFlags();
    fmc_lock();
    return true;
}

/* --------------------------------------------------------------------------
 * Boot parameter helpers
 * ------------------------------------------------------------------------ */

/**
 * @brief   Generate a minimal valid default BootParam_s.
 * @param   param  Output parameter block
 */
static void Param_SetDefault(BootParam_s *param)
{
    memset(param, 0, sizeof(BootParam_s));
    param->magic       = BOOT_PARAM_MAGIC;
    param->version     = BOOT_PARAM_VER;
    param->update_flag = UPDATE_FLAG_IDLE;
    param->app1_addr   = APP1_BASE;
    param->app2_addr   = APP2_BASE;
    param->tail_magic  = BOOT_PARAM_TAIL_MAGIC;
    param->param_crc32 = Param_CalcCrc(param);
}

/**
 * @brief   Quick sanity check: does this look like a valid BootParam_s?
 * @param   param  Parameter block to check
 * @return  true if magic / tail-magic / version look plausible
 */
static bool Param_IsMinValid(const BootParam_s *param)
{
    return (param->magic      == BOOT_PARAM_MAGIC) &&
           (param->tail_magic == BOOT_PARAM_TAIL_MAGIC) &&
           (param->version    == BOOT_PARAM_VER);
}

/**
 * @brief   Commit an OTA update to the BootLoader parameter page.
 *
 * Sets update_flag = PENDING so the BootLoader installs the cached
 * firmware on next reset.
 *
 * @param   app_size   Size of the cached App image
 * @param   app_crc32  CRC32 of the cached App image
 * @return  true on success, false on flash error
 */
static bool Param_CommitUpdate(uint32_t app_size, uint32_t app_crc32)
{
    BootParam_s main_param;
    BootParam_s backup_param;

    memcpy(m_ParamPageCache, (const void *)FLASH_PARAM_PAGE_ADDR, FLASH_PARAM_PAGE_SZ);
    memcpy(&main_param,   (const void *)FLASH_PARAM_MAIN,       sizeof(BootParam_s));
    memcpy(&backup_param, (const void *)FLASH_PARAM_BAK,        sizeof(BootParam_s));

    if (!Param_IsMinValid(&main_param))
    {
        if (Param_IsMinValid(&backup_param))
        {
            main_param = backup_param;
        }
        else
        {
            Param_SetDefault(&main_param);
        }
    }

    main_param.app_size    = app_size;
    main_param.app_crc32   = app_crc32;
    main_param.app1_addr   = APP1_BASE;
    main_param.app2_addr   = APP2_BASE;
    main_param.update_flag = UPDATE_FLAG_PENDING;
    main_param.last_error  = OTA_ERR_OK;
    main_param.param_crc32 = Param_CalcCrc(&main_param);
    backup_param = main_param;

    memcpy(&m_ParamPageCache[FLASH_PARAM_MAIN - FLASH_PARAM_PAGE_ADDR],
           &main_param, sizeof(BootParam_s));
    memcpy(&m_ParamPageCache[FLASH_PARAM_BAK - FLASH_PARAM_PAGE_ADDR],
           &backup_param, sizeof(BootParam_s));

    if (!Flash_ErasePages(FLASH_PARAM_PAGE_ADDR, FLASH_PARAM_PAGE_SZ))
    {
        return false;
    }

    return Flash_ProgBytes(FLASH_PARAM_PAGE_ADDR, m_ParamPageCache, FLASH_PARAM_PAGE_SZ);
}

/* --------------------------------------------------------------------------
 * OTA stream header parsing
 * ------------------------------------------------------------------------ */

/**
 * @brief   Validate the OTA stream header (magic, version, CRC, etc.).
 * @param   stream  Pointer to received OtaStreamHdr_s
 * @return  true if valid, false otherwise
 */
static bool Ota_ParseHdr(const OtaStreamHdr_s *stream)
{
    uint32_t crc_expect;

    if (stream == NULL)
    {
        return false;
    }

    if ((stream->magic          != OTA_STREAM_MAGIC) ||
        (stream->header_version != OTA_STREAM_HDR_VER) ||
        (stream->header_size    != sizeof(OtaStreamHdr_s)) ||
        (stream->target_addr    != APP1_BASE) ||
        (stream->image_type     != OTA_IMG_APP1) ||
        (stream->app_size       == 0U) ||
        (stream->app_size       >  APP2_SZ))
    {
        return false;
    }

    crc_expect = Crc32_Calc((const uint8_t *)stream,
                             (uint32_t)offsetof(OtaStreamHdr_s, header_crc32));
    return (crc_expect == stream->header_crc32);
}

/**
 * @brief   Begin receiving the OTA stream: erase download cache, init context.
 * @param   stream  Validated OTA stream header
 * @return  true on success, false on flash erase failure
 */
static bool Ota_BeginStream(const OtaStreamHdr_s *stream)
{
    uint32_t erase_size;

    erase_size = (stream->app_size + FLASH_PAGE_SZ - 1U) & ~(FLASH_PAGE_SZ - 1U);
    if (!Flash_ErasePages(APP_CACHE_BASE, erase_size))
    {
        return false;
    }

    m_OtaCtx.state         = OTA_ST_RECV;
    m_OtaCtx.app_version   = stream->app_version;
    m_OtaCtx.expected_size = stream->app_size;
    m_OtaCtx.expected_crc32 = stream->app_crc32;
    m_OtaCtx.recv_size     = 0U;
    m_OtaCtx.crc_acc       = 0xFFFFFFFFUL;
    m_OtaCtx.rx_overflow   = false;
    return true;
}

/**
 * @brief   Write a chunk of firmware payload to flash and update running CRC.
 * @param   payload  Firmware data
 * @param   len      Byte count
 * @return  true on success, false on overflow or flash error
 */
static bool Ota_WritePayload(const uint8_t *payload, uint32_t len)
{
    uint32_t flash_addr;

    if ((payload == NULL) || (len == 0U))
    {
        return true;
    }

    if ((m_OtaCtx.recv_size + len) > m_OtaCtx.expected_size)
    {
        Uart_Printf(DBG_UART, "OTA: size overflow\r\n");
        return false;
    }

    flash_addr = APP_CACHE_BASE + m_OtaCtx.recv_size;
    if (!Flash_ProgBytes(flash_addr, payload, len))
    {
        Uart_Printf(DBG_UART, "OTA: flash write failed\r\n");
        return false;
    }

    m_OtaCtx.crc_acc = Crc32_Update(m_OtaCtx.crc_acc, payload, len);
    m_OtaCtx.recv_size += len;
    return true;
}

/**
 * @brief   Final checks and commit when the full image has been received.
 * @return  true if the update was committed successfully
 */
static bool Ota_Finalize(void)
{
    uint32_t crc_value;

    if (m_OtaCtx.recv_size != m_OtaCtx.expected_size)
    {
        return false;
    }

    if (m_OtaCtx.rx_overflow)
    {
        Uart_Printf(DBG_UART, "OTA: rx overflow\r\n");
        return false;
    }

    crc_value = Crc32_Final(m_OtaCtx.crc_acc);
    if (crc_value != m_OtaCtx.expected_crc32)
    {
        Uart_Printf(DBG_UART, "OTA: crc mismatch, exp=0x%08X got=0x%08X\r\n",
                    m_OtaCtx.expected_crc32, crc_value);
        return false;
    }

    if (!Param_CommitUpdate(m_OtaCtx.expected_size, m_OtaCtx.expected_crc32))
    {
        Uart_Printf(DBG_UART, "OTA: param commit failed\r\n");
        return false;
    }

    return true;
}

/**
 * @brief   Drain firmware data from the FIFO and write to flash.
 *
 * Called repeatedly while in OTA_ST_RECV. When the declared app_size
 * has been fully received and verified, commits parameters and resets
 * into the BootLoader.
 *
 * @return  true while receiving, false on unrecoverable error
 */
static bool Ota_ConsumeData(void)
{
    uint8_t  payload[OTA_WRITE_CHUNK];
    uint32_t available;
    uint32_t remain;
    uint32_t chunk_size;

    while (m_OtaCtx.state == OTA_ST_RECV)
    {
        available = Fifo_Avail(&m_Fifo);
        if (available == 0U)
        {
            return true;
        }

        remain     = m_OtaCtx.expected_size - m_OtaCtx.recv_size;
        chunk_size = (available < remain) ? available : remain;
        if (chunk_size > OTA_WRITE_CHUNK)
        {
            chunk_size = OTA_WRITE_CHUNK;
        }

        (void)Fifo_Get(&m_Fifo, payload, chunk_size);
        if (!Ota_WritePayload(payload, chunk_size))
        {
            Uart_Printf(DBG_UART, "OTA: write failed at %u\r\n", m_OtaCtx.recv_size);
            return false;
        }

        if (m_OtaCtx.recv_size == m_OtaCtx.expected_size)
        {
            if (!Ota_Finalize())
            {
                return false;
            }
            Uart_Printf(DBG_UART, "OTA: done, reset to bootloader\r\n");
            __disable_irq();
            NVIC_SystemReset();
        }
    }

    return true;
}

/**
 * @brief   UART DMA IRQ handler — feed received bytes into the OTA FIFO.
 *
 * Must be called from the USART interrupt handler when new data arrives.
 *
 * @param   data  Received byte buffer
 * @param   len   Byte count
 */
void OtaUart_IrqProcess(const uint8_t *data, uint32_t len)
{
    if ((data == NULL) || (len == 0U))
    {
        return;
    }

    if (!Fifo_Put(&m_Fifo, data, len))
    {
        m_OtaCtx.rx_overflow = true;
        Uart_Printf(DBG_UART, "OTA: fifo overflow\r\n");
    }
}

/* --------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------ */

/**
 * @brief   Reset the OTA receiver state.
 */
void OtaCore_Reset(void)
{
    memset(&m_OtaCtx, 0, sizeof(m_OtaCtx));
    m_OtaCtx.state   = OTA_ST_WAIT_HDR;
    m_OtaCtx.crc_acc = 0xFFFFFFFFUL;
    Fifo_Init(&m_Fifo, m_FifoStorage, OTA_FIFO_SZ);
}

/**
 * @brief   Feed raw serial bytes into the OTA receiver.
 */
void OtaCore_FeedRaw(const uint8_t *data, uint32_t len)
{
    if ((data == NULL) || (len == 0U))
    {
        return;
    }

    if (!Fifo_Put(&m_Fifo, data, len))
    {
        m_OtaCtx.rx_overflow = true;
        Uart_Printf(DBG_UART, "OTA: fifo overflow\r\n");
    }
}

/**
 * @brief   Execute the OTA receive state machine (call from main loop).
 */
void OtaCore_Poll(void)
{
    OtaStreamHdr_s stream_header;
    uint32_t       available;

    while (1)
    {
        if (m_OtaCtx.state == OTA_ST_RECV)
        {
            if (!Ota_ConsumeData())
            {
                OtaCore_Reset();
            }
            return;
        }

        available = Fifo_Avail(&m_Fifo);
        if (available < sizeof(uint32_t))
        {
            return;
        }

        (void)Fifo_Peek(&m_Fifo, (uint8_t *)&stream_header.magic, sizeof(stream_header.magic));
        if (stream_header.magic != OTA_STREAM_MAGIC)
        {
            Fifo_Drop(&m_Fifo, 1U);
            continue;
        }

        if (available < sizeof(OtaStreamHdr_s))
        {
            return;
        }

        (void)Fifo_Peek(&m_Fifo, (uint8_t *)&stream_header, sizeof(stream_header));
        if (!Ota_ParseHdr(&stream_header))
        {
            Fifo_Drop(&m_Fifo, 1U);
            continue;
        }

        Fifo_Drop(&m_Fifo, (uint32_t)sizeof(OtaStreamHdr_s));
        if (!Ota_BeginStream(&stream_header))
        {
            Uart_Printf(DBG_UART, "OTA: begin failed\r\n");
            OtaCore_Reset();
            return;
        }
    }
}

/**
 * @brief   Request a fast bootloader upgrade (no payload, just a flag).
 * @return  1 on success, 0 on failure
 */
uint8_t OtaCore_ReqUpgrade(void)
{
    BootParam_s main_param;
    BootParam_s backup_param;

    memcpy(m_ParamPageCache, (const void *)FLASH_PARAM_PAGE_ADDR, FLASH_PARAM_PAGE_SZ);
    memcpy(&main_param,   (const void *)FLASH_PARAM_MAIN,       sizeof(BootParam_s));
    memcpy(&backup_param, (const void *)FLASH_PARAM_BAK,        sizeof(BootParam_s));

    if (!Param_IsMinValid(&main_param))
    {
        if (Param_IsMinValid(&backup_param))
        {
            main_param = backup_param;
        }
        else
        {
            Param_SetDefault(&main_param);
        }
    }

    main_param.update_flag = UPDATE_FLAG_FAST;
    main_param.app_size    = 0UL;
    main_param.app_crc32   = 0UL;
    main_param.app1_addr   = APP1_BASE;
    main_param.app2_addr   = APP2_BASE;
    main_param.last_error  = OTA_ERR_OK;
    main_param.param_crc32 = Param_CalcCrc(&main_param);
    backup_param = main_param;

    memcpy(&m_ParamPageCache[FLASH_PARAM_MAIN - FLASH_PARAM_PAGE_ADDR],
           &main_param, sizeof(BootParam_s));
    memcpy(&m_ParamPageCache[FLASH_PARAM_BAK - FLASH_PARAM_PAGE_ADDR],
           &backup_param, sizeof(BootParam_s));

    if (!Flash_ErasePages(FLASH_PARAM_PAGE_ADDR, FLASH_PARAM_PAGE_SZ))
    {
        return 0U;
    }

    return Flash_ProgBytes(FLASH_PARAM_PAGE_ADDR, m_ParamPageCache, FLASH_PARAM_PAGE_SZ) ? 1U : 0U;
}

/**
 * @brief   Save current APP communication parameters to the BootLoader page.
 *
 * @param   device_id  Current device ID (0x0001..0xFFFE)
 * @param   baud_code  Baud rate map code
 * @return  1 on success, 0 on failure
 */
uint8_t OtaCore_SaveCommParams(uint16_t device_id, uint8_t baud_code)
{
    BootParam_s main_param;
    BootParam_s backup_param;

    memcpy(m_ParamPageCache, (const void *)FLASH_PARAM_PAGE_ADDR, FLASH_PARAM_PAGE_SZ);
    memcpy(&main_param,   (const void *)FLASH_PARAM_MAIN,       sizeof(BootParam_s));
    memcpy(&backup_param, (const void *)FLASH_PARAM_BAK,        sizeof(BootParam_s));

    if (!Param_IsMinValid(&main_param))
    {
        if (Param_IsMinValid(&backup_param))
        {
            main_param = backup_param;
        }
        else
        {
            Param_SetDefault(&main_param);
        }
    }

    if ((main_param.comm_baud_code == (uint32_t)baud_code) &&
        (main_param.comm_device_id == (uint32_t)device_id))
    {
        return 1U;
    }

    main_param.comm_baud_code = (uint32_t)baud_code;
    main_param.comm_device_id = (uint32_t)device_id;
    main_param.app1_addr      = APP1_BASE;
    main_param.app2_addr      = APP2_BASE;
    main_param.param_crc32    = Param_CalcCrc(&main_param);
    backup_param = main_param;

    memcpy(&m_ParamPageCache[FLASH_PARAM_MAIN - FLASH_PARAM_PAGE_ADDR],
           &main_param, sizeof(BootParam_s));
    memcpy(&m_ParamPageCache[FLASH_PARAM_BAK - FLASH_PARAM_PAGE_ADDR],
           &backup_param, sizeof(BootParam_s));

    if (!Flash_ErasePages(FLASH_PARAM_PAGE_ADDR, FLASH_PARAM_PAGE_SZ))
    {
        return 0U;
    }

    return Flash_ProgBytes(FLASH_PARAM_PAGE_ADDR, m_ParamPageCache, FLASH_PARAM_PAGE_SZ) ? 1U : 0U;
}
