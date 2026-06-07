/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：SPI_Flash.c
 * 作者: Jianchuan Wang & Tao Liu @ GigaDevice
 * 平台: 2025CIMC IHD-V04
 * 版本: Jianchuan Wang     2025/4/20     V0.01    original
************************************************************/
#include "SPI_FLASH.h"

/* SPI Flash 操作指令定义 */
#define WRITE            0x02     /* 页编程指令（写入数据到存储器） */
#define WRSR             0x01     /* 写状态寄存器指令 */
#define WREN             0x06     /* 写使能指令（操作前必须发送） */

#define READ             0x03     /* 读数据指令（从存储器读取数据） */
#define RDSR             0x05     /* 读状态寄存器指令 */
#define RDID             0x9F     /* 读器件ID指令（获取制造商和设备ID） */
#define SE               0x20     /* 扇区擦除指令（擦除4KB扇区） */
#define BE               0xC7     /* 整片擦除指令（擦除整个芯片） */

#define WIP_FLAG         0x01     /* 状态寄存器中的写操作忙标志位（Busy bit） */
#define DUMMY_BYTE       0xA5     /* 用于产生时钟的哑字节（任意值均可） */

/*!
    \brief      初始化SPI0的GPIO及外设参数，并配置Flash片选引脚
    \param[in]  无
    \param[out] 无
    \retval     无
*/
void spi_flash_init(void)
{
    spi_parameter_struct spi_init_struct;  /* SPI初始化结构体 */

    /* 使能GPIOB和SPI0的外设时钟 */
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_SPI0);

    /* 配置SPI0的复用引脚：PB3(SCK)、PB4(MISO)、PB5(MOSI) */
    gpio_af_set(GPIOB, GPIO_AF_5, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);

    /* 配置Flash片选引脚：PA15，推挽输出，无上下拉 */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_15);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_15);

    /* 片选信号无效（高电平） */
    SPI_FLASH_CS_HIGH();

    /* 配置SPI0工作参数 */
    spi_init_struct.trans_mode         = SPI_TRANSMODE_FULLDUPLEX;   /* 全双工模式 */
    spi_init_struct.device_mode        = SPI_MASTER;                /* 主机模式 */
    spi_init_struct.frame_size         = SPI_FRAMESIZE_8BIT;        /* 8位数据帧 */
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE; /* 时钟空闲低，第1个边沿采样 */
    spi_init_struct.nss                = SPI_NSS_SOFT;              /* 软件NSS管理 */
    spi_init_struct.prescale           = SPI_PSC_8;                 /* 8分频（SPI时钟 = 系统时钟/8） */
    spi_init_struct.endian             = SPI_ENDIAN_MSB;            /* 高位在前 */
    spi_init(SPI0, &spi_init_struct);

    /* 使能SPI0外设 */
    spi_enable(SPI0);
}

/*!
    \brief      擦除指定的Flash扇区（4KB）
    \param[in]  sector_addr: 要擦除的扇区起始地址
    \param[out] 无
    \retval     无
*/
void spi_flash_sector_erase(uint32_t sector_addr)
{
    /* 发送写使能指令 */
    spi_flash_write_enable();

    /* 拉低片选，选中Flash */
    SPI_FLASH_CS_LOW();
    /* 发送扇区擦除指令 */
    spi_flash_send_byte(SE);
    /* 发送24位地址：高字节 */
    spi_flash_send_byte((sector_addr & 0xFF0000) >> 16);
    /* 中字节 */
    spi_flash_send_byte((sector_addr & 0xFF00) >> 8);
    /* 低字节 */
    spi_flash_send_byte(sector_addr & 0xFF);
    /* 拉高片选，释放Flash */
    SPI_FLASH_CS_HIGH();

    /* 等待擦除操作完成 */
    spi_flash_wait_for_write_end();
}

/*!
    \brief      擦除Flash中指定长度区域（任意对齐方式，非扇区对齐时使用备份恢复策略）
    \param[in]  sector_addr: 擦除起始地址（可以是任意字节地址）
    \param[in]  num_byte_to_erase: 要擦除的字节数
    \param[out] 无
    \retval     无
    \note       内部会处理非扇区对齐的情况，通过读取-擦除-写回的方式保护不需要擦除的数据
*/
void spi_flash_buffer_erase(uint32_t sector_addr, uint32_t num_byte_to_erase)
{
    /* 定义扇区大小的缓冲区用于暂存数据 */
    uint8_t buffer_data[SPI_FLASH_SECTOR_SIZE] = { 0 };
    uint8_t buffer_data1[SPI_FLASH_SECTOR_SIZE] = { 0 };
    uint8_t num_of_sector = 0, num_of_single = 0, addr = 0, count = 0;

    /* 计算起始地址在扇区内的偏移 */
    addr = sector_addr % SPI_FLASH_SECTOR_SIZE;
    /* 计算该页内从起始地址到页末的剩余空间 */
    count = SPI_FLASH_PAGE_SIZE - addr;
    /* 计算需要擦除的完整扇区数 */
    num_of_sector = num_byte_to_erase / SPI_FLASH_SECTOR_SIZE;
    /* 计算剩余不足一个扇区的字节数 */
    num_of_single = num_byte_to_erase % SPI_FLASH_SECTOR_SIZE;

    /* 情况1：起始地址是扇区对齐的 */
    if (0 == addr)
    {
        /* 擦除完整的扇区 */
        while (num_of_sector--)
        {
            spi_flash_sector_erase(sector_addr);
            sector_addr += SPI_FLASH_PAGE_SIZE;   /* 注意：此处用页大小增加地址，每次移动256字节，
                                                     实际上一个扇区通常是4096字节，需结合宏定义确认 */
        }
        /* 处理剩余不足一个扇区的部分 */
        if (0 != num_of_single)
        {
            /* 先读出扇区中擦除范围之后的数据 */
            spi_flash_buffer_read(buffer_data, sector_addr + num_of_single, SPI_FLASH_SECTOR_SIZE - num_of_single);
            /* 擦除整个扇区 */
            spi_flash_sector_erase(sector_addr);
            /* 将保留的数据写回 */
            spi_flash_buffer_write(buffer_data, sector_addr + num_of_single, SPI_FLASH_SECTOR_SIZE - num_of_single);
        }
    }
    else /* 情况2：起始地址是非扇区对齐的 */
    {
        /* 如果待擦除的数据全部在当前页内 */
        if (num_byte_to_erase < count)
        {
            /* 读出扇区中位于擦除区域前的数据 */
            spi_flash_buffer_read(buffer_data, num_of_sector * SPI_FLASH_SECTOR_SIZE, addr);
            /* 读出扇区中位于擦除区域后的数据 */
            spi_flash_buffer_read(buffer_data1, num_of_sector * SPI_FLASH_SECTOR_SIZE + addr + num_byte_to_erase,
                                  SPI_FLASH_SECTOR_SIZE - addr - num_byte_to_erase);
            /* 擦除整个扇区 */
            spi_flash_sector_erase(sector_addr);
            /* 将保留的前部数据写回 */
            spi_flash_buffer_write(buffer_data, num_of_sector * SPI_FLASH_SECTOR_SIZE, addr);
            /* 将保留的后部数据写回 */
            spi_flash_buffer_write(buffer_data1, num_of_sector * SPI_FLASH_SECTOR_SIZE + addr + num_byte_to_erase,
                                   SPI_FLASH_SECTOR_SIZE - addr - num_byte_to_erase);
        }
        else /* 待擦除数据跨越多个扇区 */
        {
            /* 先处理第一个扇区中非对齐的部分 */
            /* 读出扇区前部（擦除区域之前）的数据 */
            spi_flash_buffer_read(buffer_data, num_of_sector * SPI_FLASH_SECTOR_SIZE, addr);
            /* 擦除第一个扇区 */
            spi_flash_sector_erase(sector_addr);
            /* 写回前部数据 */
            spi_flash_buffer_write(buffer_data, num_of_sector * SPI_FLASH_SECTOR_SIZE, addr);

            /* 调整剩余长度：减去首个扇区已处理的部分 */
            num_byte_to_erase -= addr;
            /* 重新计算完整的扇区数和剩余字节数 */
            num_of_sector = num_byte_to_erase / SPI_FLASH_SECTOR_SIZE;
            num_of_single = num_byte_to_erase % SPI_FLASH_SECTOR_SIZE;
            sector_addr += count;  /* 移动到下一个对齐的扇区起始处 */

            /* 擦除中间的完整扇区 */
            while (num_of_sector--)
            {
                spi_flash_sector_erase(sector_addr);
                sector_addr += SPI_FLASH_PAGE_SIZE;
            }
            /* 处理最后不足一个扇区的部分 */
            if (0 != num_of_single)
            {
                /* 读出最后一个扇区中擦除区域之后的数据 */
                spi_flash_buffer_read(buffer_data, sector_addr + num_of_single, SPI_FLASH_SECTOR_SIZE - num_of_single);
                /* 擦除该扇区 */
                spi_flash_sector_erase(sector_addr);
                /* 写回保留的后部数据 */
                spi_flash_buffer_write(buffer_data, sector_addr + num_of_single, SPI_FLASH_SECTOR_SIZE - num_of_single);
            }
        }
    }
}

/*!
    \brief      擦除整个Flash芯片（整片擦除）
    \param[in]  无
    \param[out] 无
    \retval     无
*/
void spi_flash_bulk_erase(void)
{
    /* 发送写使能 */
    spi_flash_write_enable();

    /* 拉低片选 */
    SPI_FLASH_CS_LOW();
    /* 发送整片擦除指令 */
    spi_flash_send_byte(BE);
    /* 释放片选 */
    SPI_FLASH_CS_HIGH();

    /* 等待擦除完成 */
    spi_flash_wait_for_write_end();
}

/*!
    \brief      向Flash写入不超过一页（通常256字节）的数据
    \param[in]  pbuffer: 源数据缓冲区指针
    \param[in]  write_addr: 写入的起始地址
    \param[in]  num_byte_to_write: 要写入的字节数（不能跨越页边界）
    \param[out] 无
    \retval     无
*/
void spi_flash_page_write(uint8_t* pbuffer, uint32_t write_addr, uint16_t num_byte_to_write)
{
    /* 写使能 */
    spi_flash_write_enable();

    /* 选中Flash */
    SPI_FLASH_CS_LOW();

    /* 发送页编程指令 */
    spi_flash_send_byte(WRITE);
    /* 发送24位地址 */
    spi_flash_send_byte((write_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((write_addr & 0xFF00) >> 8);
    spi_flash_send_byte(write_addr & 0xFF);

    /* 循环发送待写入的数据 */
    while (num_byte_to_write--) {
        spi_flash_send_byte(*pbuffer);
        pbuffer++;
    }

    /* 释放片选 */
    SPI_FLASH_CS_HIGH();

    /* 等待写入完成 */
    spi_flash_wait_for_write_end();
}

/*!
    \brief      向Flash写入任意长度的数据（自动处理跨页写入）
    \param[in]  pbuffer: 源数据缓冲区指针
    \param[in]  write_addr: 写入的起始地址
    \param[in]  num_byte_to_write: 要写入的总字节数
    \param[out] 无
    \retval     无
*/
void spi_flash_buffer_write(uint8_t* pbuffer, uint32_t write_addr, uint32_t num_byte_to_write)
{
    uint8_t num_of_page = 0, num_of_single = 0, addr = 0, count = 0;

    /* 计算起始地址在页内的偏移 */
    addr = write_addr % SPI_FLASH_PAGE_SIZE;
    /* 本页剩余的连续空间大小 */
    count = SPI_FLASH_PAGE_SIZE - addr;
    /* 需要写入的完整页数 */
    num_of_page = num_byte_to_write / SPI_FLASH_PAGE_SIZE;
    /* 剩余不足一页的字节数 */
    num_of_single = num_byte_to_write % SPI_FLASH_PAGE_SIZE;

    /* 如果起始地址页对齐 */
    if (0 == addr)
    {
        /* 先写完整的页 */
        while (num_of_page--)
        {
            spi_flash_page_write(pbuffer, write_addr, SPI_FLASH_PAGE_SIZE);
            write_addr += SPI_FLASH_PAGE_SIZE;
            pbuffer    += SPI_FLASH_PAGE_SIZE;
        }
        /* 再写剩余不足一页的部分 */
        if (0 != num_of_single)
            spi_flash_page_write(pbuffer, write_addr, num_of_single);
    }
    else /* 起始地址非页对齐 */
    {
        /* 如果数据总量小于当前页剩余空间，则一次页写入即可 */
        if (num_byte_to_write < count)
        {
            spi_flash_page_write(pbuffer, write_addr, num_byte_to_write);
        }
        else
        {
            /* 先写当前页剩余的部分 */
            spi_flash_page_write(pbuffer, write_addr, count);

            /* 重新计算剩余数据对应的完整页和零头 */
            num_of_page = (num_byte_to_write - count) / SPI_FLASH_PAGE_SIZE;
            num_of_single = (num_byte_to_write - count) % SPI_FLASH_PAGE_SIZE;
            write_addr += count;
            pbuffer    += count;

            /* 写入中间的完整页 */
            while (num_of_page--)
            {
                spi_flash_page_write(pbuffer, write_addr, SPI_FLASH_PAGE_SIZE);
                write_addr += SPI_FLASH_PAGE_SIZE;
                pbuffer    += SPI_FLASH_PAGE_SIZE;
            }

            /* 写入最后不足一页的数据 */
            if (0 != num_of_single)
                spi_flash_page_write(pbuffer, write_addr, num_of_single);
        }
    }
}

/*!
    \brief      从Flash中读取指定长度的数据到缓冲区
    \param[in]  pbuffer: 接收数据的缓冲区指针
    \param[in]  read_addr: 读取起始地址
    \param[in]  num_byte_to_read: 要读取的字节数
    \param[out] 无
    \retval     无
*/
void spi_flash_buffer_read(uint8_t* pbuffer, uint32_t read_addr, uint16_t num_byte_to_read)
{
    /* 选中Flash */
    SPI_FLASH_CS_LOW();

    /* 发送读数据指令 */
    spi_flash_send_byte(READ);

    /* 发送24位起始地址 */
    spi_flash_send_byte((read_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((read_addr & 0xFF00) >> 8);
    spi_flash_send_byte(read_addr & 0xFF);

    /* 循环读取指定长度的数据 */
    while (num_byte_to_read--) {
        *pbuffer = spi_flash_send_byte(DUMMY_BYTE);  /* 发送哑字节以产生读取时钟，返回读到的数据 */
        pbuffer++;
    }

    /* 释放片选 */
    SPI_FLASH_CS_HIGH();
}

/*!
    \brief      读取Flash器件ID（制造商ID和设备ID）
    \param[in]  无
    \param[out] 无
    \retval     24位器件ID值
*/
uint32_t spi_flash_read_id(void)
{
    uint32_t temp = 0, temp0 = 0, temp1 = 0, temp2 = 0;

    /* 片选拉低 */
    SPI_FLASH_CS_LOW();

    /* 发送读ID指令（0x9F） */
    spi_flash_send_byte(0x9F);

    /* 依次读取三个字节：制造商ID、存储器类型、容量 */
    temp0 = spi_flash_send_byte(DUMMY_BYTE);
    temp1 = spi_flash_send_byte(DUMMY_BYTE);
    temp2 = spi_flash_send_byte(DUMMY_BYTE);

    /* 片选拉高 */
    SPI_FLASH_CS_HIGH();

    /* 拼合成24位ID */
    temp = (temp0 << 16) | (temp1 << 8) | temp2;

    return temp;
}

/*!
    \brief      启动一次读数据序列（发送指令和地址，但不读取数据）
    \param[in]  read_addr: 要读取的起始地址
    \param[out] 无
    \retval     无
    \note       通常用于快速连续读取，由后续函数单独读取每个字节
*/
void spi_flash_start_read_sequence(uint32_t read_addr)
{
    /* 选中Flash */
    SPI_FLASH_CS_LOW();

    /* 发送读指令 */
    spi_flash_send_byte(READ);

    /* 发送24位起始地址 */
    spi_flash_send_byte((read_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((read_addr & 0xFF00) >> 8);
    spi_flash_send_byte(read_addr & 0xFF);
}

/*!
    \brief      在已启动的读序列中读取一个字节
    \param[in]  无
    \param[out] 无
    \retval     从Flash读取的一个字节
*/
uint8_t spi_flash_read_byte(void)
{
    return(spi_flash_send_byte(DUMMY_BYTE));
}

/*!
    \brief      通过SPI发送一个字节，并返回同时接收到的字节
    \param[in]  byte: 要发送的字节
    \param[out] 无
    \retval     在发送过程中接收到的字节
*/
uint8_t spi_flash_send_byte(uint8_t byte)
{
    /* 等待发送缓冲区空 */
    while (RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_TBE));

    /* 发送字节 */
    spi_i2s_data_transmit(SPI0, byte);

    /* 等待接收缓冲区非空 */
    while (RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE));

    /* 返回接收到的字节 */
    return(spi_i2s_data_receive(SPI0));
}

/*!
    \brief      通过SPI发送一个半字（16位），并返回接收到的半字
    \param[in]  half_word: 要发送的半字
    \param[out] 无
    \retval     接收到的半字
*/
uint16_t spi_flash_send_halfword(uint16_t half_word)
{
    /* 等待发送缓冲区空 */
    while (RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_TBE));

    /* 发送半字 */
    spi_i2s_data_transmit(SPI0, half_word);

    /* 等待接收缓冲区非空 */
    while (RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE));

    /* 返回接收到的半字 */
    return spi_i2s_data_receive(SPI0);
}

/*!
    \brief      使能Flash的写操作（发送写使能指令）
    \param[in]  无
    \param[out] 无
    \retval     无
*/
void spi_flash_write_enable(void)
{
    /* 选中Flash */
    SPI_FLASH_CS_LOW();

    /* 发送写使能指令 */
    spi_flash_send_byte(WREN);

    /* 释放片选 */
    SPI_FLASH_CS_HIGH();
}

/*!
    \brief      查询Flash状态寄存器中的忙标志，等待写/擦除操作完成
    \param[in]  无
    \param[out] 无
    \retval     无
*/
void spi_flash_wait_for_write_end(void)
{
    uint8_t flash_status = 0;

    /* 选中Flash */
    SPI_FLASH_CS_LOW();

    /* 发送读状态寄存器指令 */
    spi_flash_send_byte(RDSR);

    /* 循环读取状态寄存器，直到忙标志位（WIP）变为0 */
    do {
        /* 发送哑字节读取状态寄存器值 */
        flash_status = spi_flash_send_byte(DUMMY_BYTE);
    } while ((flash_status & WIP_FLAG) == SET);  /* SET通常定义为1 */

    /* 释放片选 */
    SPI_FLASH_CS_HIGH();
}
