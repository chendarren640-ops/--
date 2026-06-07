#include "bsp_flash.h"

/* ========== 内部辅助函数 ========== */

static void flash_clear_flags(void)
{
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR);
}

static int flash_erase_sector(uint32_t sector)
{
    fmc_state_enum state;
    fmc_unlock();
    flash_clear_flags();
    state = fmc_sector_erase(sector);
    flash_clear_flags();
    fmc_lock();
    return (state == FMC_READY) ? 0 : -1;
}

static int flash_program(uint32_t addr, const uint8_t *data, uint32_t len)
{
    uint32_t write_addr, remain, i;
    uint32_t word;
    uint8_t temp[4];

    if (data == NULL || len == 0) return -1;
    if (addr % 4 != 0) return -1;

    write_addr = addr;
    remain = len;
    i = 0;

    fmc_unlock();
    flash_clear_flags();

    while (remain > 0)
    {
        temp[0] = 0xFF; temp[1] = 0xFF; temp[2] = 0xFF; temp[3] = 0xFF;

        if (remain >= 4)
        {
            temp[0] = data[i]; temp[1] = data[i+1];
            temp[2] = data[i+2]; temp[3] = data[i+3];
            remain -= 4; i += 4;
        }
        else
        {
            uint32_t j;
            for (j = 0; j < remain; j++)
                temp[j] = data[i+j];
            i += remain; remain = 0;
        }

        word = ((uint32_t)temp[0]) | ((uint32_t)temp[1] << 8) |
               ((uint32_t)temp[2] << 16) | ((uint32_t)temp[3] << 24);

        if (fmc_word_program(write_addr, word) != FMC_READY)
        {
            fmc_lock();
            return -2;
        }
        write_addr += 4;
    }

    flash_clear_flags();
    fmc_lock();
    return 0;
}

/* ========== 对外接口 ========== */

uint8_t BSP_FLASH_Erase(uint32_t start_addr, uint32_t size)
{
    static const uint32_t sector_starts[] = {
        0x08000000, 0x08004000, 0x08008000, 0x0800C000, 0x08010000,
        0x08020000, 0x08040000, 0x08060000, 0x08080000, 0x080A0000,
        0x080C0000, 0x080E0000
    };
    static const uint32_t sector_sizes[] = {
        0x4000, 0x4000, 0x4000, 0x4000, 0x10000,
        0x20000, 0x20000, 0x20000, 0x20000, 0x20000,
        0x20000, 0x20000
    };
    const uint32_t num_sectors = sizeof(sector_starts) / sizeof(sector_starts[0]);
    uint32_t end_addr = start_addr + size - 1;
    uint32_t i;

    for (i = 0; i < num_sectors; i++)
    {
        uint32_t s_start = sector_starts[i];
        uint32_t s_end   = s_start + sector_sizes[i] - 1;

        if (start_addr <= s_end && end_addr >= s_start)
        {
            if (flash_erase_sector(i) != 0)
                return 0;
        }
    }
    return 1;
}

uint8_t BSP_FLASH_Write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    /* 允许写入参数区和告警区等合法Flash区域 */
    if (addr < 0x08000000U || addr + len > 0x08100000U)
        return 0;
    return (flash_program(addr, data, len) == 0) ? 1 : 0;
}

void BSP_FLASH_Read(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++)
        data[i] = *(volatile uint8_t *)(addr + i);
}

uint32_t BSP_FLASH_ReadWord(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

uint8_t BSP_FLASH_IsAppValid(uint32_t app_addr)
{
    uint32_t stack_top = *(volatile uint32_t *)app_addr;
    return (stack_top >= 0x20000000 && stack_top <= 0x20040000) ? 1 : 0;
}


