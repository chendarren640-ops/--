/* SPDX-License-Identifier: MIT */

/**
 * @file    flash_fs_app.c
 * @brief   LittleFS filesystem on external SPI flash (GD25Qxx).
 *
 * Provides mount, test, read, and write operations over the littlefs
 * library backed by the GD25Qxx SPI NOR flash via the lfs_port layer.
 */

#include "board_defs.h"
#include "lfs.h"
#include "lfs_port.h"
#include "usart_app.h"

/** littlefs filesystem instance. */
static lfs_t m_Lfs;

/** littlefs storage configuration. */
static struct lfs_config m_LfsCfg;

/** Mount flag: 1 when the filesystem is ready, 0 otherwise. */
static uint8_t m_LfsMounted;

/**
 * @brief Initialise and mount the littlefs filesystem on external flash.
 * @return 0 on success, negative error code on failure.
 */
int FlashFs_Init(void)
{
    int ret;

    ret = lfs_storage_init(&m_LfsCfg);
    if (ret < 0)
    {
        Uart_Printf(DEBUG_USART, "LFS: storage init failed (%d)\r\n", ret);
        return ret;
    }

    ret = lfs_mount(&m_Lfs, &m_LfsCfg);
    if (ret < 0)
    {
        Uart_Printf(DEBUG_USART, "LFS: mount failed (%d), format...\r\n", ret);

        ret = lfs_format(&m_Lfs, &m_LfsCfg);
        if (ret < 0)
        {
            Uart_Printf(DEBUG_USART, "LFS: format failed (%d)\r\n", ret);
            return ret;
        }

        ret = lfs_mount(&m_Lfs, &m_LfsCfg);
        if (ret < 0)
        {
            Uart_Printf(DEBUG_USART, "LFS: remount failed (%d)\r\n", ret);
            return ret;
        }
    }

    m_LfsMounted = 1U;
    Uart_Printf(DEBUG_USART, "LFS: mount ok\r\n");
    return 0;
}

/**
 * @brief Run a simple write/read/verify self-test on the littlefs filesystem.
 */
void FlashFs_Test(void)
{
    lfs_file_t file;
    const char msg[] = "littlefs on gd25qxx ok";
    char readback[32] = {0};
    int ret;
    lfs_ssize_t len;

    if (0U == m_LfsMounted)
    {
        Uart_Printf(DEBUG_USART, "LFS: test skipped, not mounted\r\n");
        return;
    }

    ret = lfs_file_open(&m_Lfs, &file, "lfs_test.txt",
                        LFS_O_CREAT | LFS_O_TRUNC | LFS_O_WRONLY);
    if (ret < 0)
    {
        Uart_Printf(DEBUG_USART, "LFS: open write failed (%d)\r\n", ret);
        return;
    }

    len = lfs_file_write(&m_Lfs, &file, msg, sizeof(msg));
    ret = lfs_file_close(&m_Lfs, &file);
    if ((len != (lfs_ssize_t)sizeof(msg)) || (ret < 0))
    {
        Uart_Printf(DEBUG_USART, "LFS: write failed (len=%d, ret=%d)\r\n", len, ret);
        return;
    }

    ret = lfs_file_open(&m_Lfs, &file, "lfs_test.txt", LFS_O_RDONLY);
    if (ret < 0)
    {
        Uart_Printf(DEBUG_USART, "LFS: open read failed (%d)\r\n", ret);
        return;
    }

    len = lfs_file_read(&m_Lfs, &file, readback, sizeof(readback));
    ret = lfs_file_close(&m_Lfs, &file);
    if ((len == (lfs_ssize_t)sizeof(msg)) && (0 == memcmp(readback, msg, sizeof(msg))) && (ret >= 0))
    {
        Uart_Printf(DEBUG_USART, "LFS: test ok, %s\r\n", readback);
    }
    else
    {
        Uart_Printf(DEBUG_USART, "LFS: verify failed (len=%d, ret=%d)\r\n", len, ret);
    }
}

/**
 * @brief Read a file from the littlefs filesystem into a buffer.
 * @param path    Null-terminated file path.
 * @param buffer  Destination buffer.
 * @param size    Maximum number of bytes to read.
 * @return        Number of bytes read on success, negative error on failure.
 */
int FlashFs_Read(const char *path, void *buffer, uint32_t size)
{
    lfs_file_t file;
    lfs_ssize_t len;
    int ret;

    if ((0U == m_LfsMounted) || (path == NULL) || (buffer == NULL))
    {
        return -1;
    }

    ret = lfs_file_open(&m_Lfs, &file, path, LFS_O_RDONLY);
    if (ret < 0)
    {
        return ret;
    }

    len = lfs_file_read(&m_Lfs, &file, buffer, size);
    ret = lfs_file_close(&m_Lfs, &file);
    if (ret < 0)
    {
        return ret;
    }

    return (int)len;
}

/**
 * @brief Write data to a file on the littlefs filesystem.
 * @param path    Null-terminated file path.
 * @param buffer  Source buffer containing the data to write.
 * @param size    Number of bytes to write.
 * @return        Number of bytes written on success, negative error on failure.
 */
int FlashFs_Write(const char *path, const void *buffer, uint32_t size)
{
    lfs_file_t file;
    lfs_ssize_t len;
    int ret;

    if ((0U == m_LfsMounted) || (path == NULL) || (buffer == NULL))
    {
        return -1;
    }

    ret = lfs_file_open(&m_Lfs, &file, path, LFS_O_CREAT | LFS_O_TRUNC | LFS_O_WRONLY);
    if (ret < 0)
    {
        return ret;
    }

    len = lfs_file_write(&m_Lfs, &file, buffer, size);
    if (len == (lfs_ssize_t)size)
    {
        ret = lfs_file_sync(&m_Lfs, &file);
    }
    if (ret >= 0)
    {
        ret = lfs_file_close(&m_Lfs, &file);
    }
    else
    {
        (void)lfs_file_close(&m_Lfs, &file);
    }

    if ((len != (lfs_ssize_t)size) || (ret < 0))
    {
        return -1;
    }

    return (int)len;
}

/**
 * @brief ARM standard assertion handler.
 *
 * Called by the toolchain when an assertion fails.  Prints the
 * assertion details over the debug UART and loops forever.
 *
 * @param expr  The failed expression string (may be NULL).
 * @param file  Source file name (may be NULL).
 * @param line  Source line number.
 */
void __aeabi_assert(const char *expr, const char *file, int line)
{
    Uart_Printf(DEBUG_USART,
                "ASSERT: %s, file: %s, line: %d\r\n",
                (NULL != expr) ? expr : "?",
                (NULL != file) ? file : "?",
                line);
    while (1)
    {
    }
}
