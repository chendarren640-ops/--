/*
 * bsp_flash.c
 * GD32F470 内部 Flash 驱动
 */

#include "bsp_flash.h"

static void bsp_flash_clear_flags(void)
{
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR);
}

void bsp_flash_unlock(void)
{
    fmc_unlock();
}

void bsp_flash_lock(void)
{
    fmc_lock();
}

int bsp_flash_erase_sector(uint32_t sector)
{
    fmc_state_enum state;
    fmc_unlock();
    bsp_flash_clear_flags();
    state = fmc_sector_erase(sector);
    bsp_flash_clear_flags();
    fmc_lock();
    return (state == FMC_READY) ? 0 : -1;
}

int bsp_flash_program_word(uint32_t address, uint32_t data)
{
    fmc_state_enum state;
    fmc_unlock();
    bsp_flash_clear_flags();
    state = fmc_word_program(address, data);
    bsp_flash_clear_flags();
    fmc_lock();
    return (state == FMC_READY) ? 0 : -1;
}

int bsp_flash_program(uint32_t address, const uint8_t *data, uint32_t length)
{
    uint32_t i;
    uint32_t word;
    uint32_t write_addr;
    uint32_t remain;
    uint8_t temp[4];

    if(data == 0) return -1;

    write_addr = address;
    remain = length;
    i = 0;

    fmc_unlock();
    bsp_flash_clear_flags();

    while(remain > 0)
    {
        temp[0] = 0xFF; temp[1] = 0xFF; temp[2] = 0xFF; temp[3] = 0xFF;

        if(remain >= 4)
        {
            temp[0] = data[i+0]; temp[1] = data[i+1];
            temp[2] = data[i+2]; temp[3] = data[i+3];
            remain -= 4; i += 4;
        }
        else
        {
            uint32_t j;
            for(j = 0; j < remain; j++) temp[j] = data[i+j];
            i += remain; remain = 0;
        }

        word = ((uint32_t)temp[0]) | ((uint32_t)temp[1] << 8) |
               ((uint32_t)temp[2] << 16) | ((uint32_t)temp[3] << 24);

        if(fmc_word_program(write_addr, word) != FMC_READY)
        {
            fmc_lock();
            return -2;
        }
        write_addr += 4;
    }

    bsp_flash_clear_flags();
    fmc_lock();
    return 0;
}

void bsp_flash_read(uint32_t address, uint8_t *data, uint32_t length)
{
    uint32_t i;
    if(data == 0) return;
    for(i = 0; i < length; i++)
    {
        data[i] = *(volatile uint8_t *)(address + i);
    }
}

uint32_t bsp_flash_read_word(uint32_t address)
{
    return *(volatile uint32_t *)address;
}

/*
 * BSP_FLASH_Erase - 擦除指定地址范围的 Flash 扇区
 * GD32F470 扇区布局:
 *   Sector 0-3:  各 16KB  (0x08000000 ~ 0x0800FFFF)
 *   Sector 4:    64KB     (0x08010000 ~ 0x0801FFFF)
 *   Sector 5-6:  各 128KB (0x08020000 ~ 0x0805FFFF)
 *   Sector 7-11: 各 128KB (0x08060000 ~ 0x080FFFFF)
 */
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

    for(i = 0; i < num_sectors; i++)
    {
        uint32_t s_start = sector_starts[i];
        uint32_t s_end   = s_start + sector_sizes[i] - 1;

        if(start_addr <= s_end && end_addr >= s_start)
        {
            if(bsp_flash_erase_sector(i) != 0)
                return 0;
        }
    }
    return 1;
}

uint8_t BSP_FLASH_Write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    return (bsp_flash_program(addr, data, len) == 0) ? 1 : 0;
}

void BSP_FLASH_Read(uint32_t addr, uint8_t *data, uint32_t len)
{
    bsp_flash_read(addr, data, len);
}

uint8_t BSP_FLASH_IsAppValid(uint32_t app_addr)
{
    uint32_t stack_top = *(volatile uint32_t *)app_addr;
    return (stack_top >= 0x20000000 && stack_top <= 0x20040000) ? 1 : 0;
}
