 /************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：SPI_Flash.c
************************************************************/
#include "SPI_FLASH.h"

/* SPI Flash 操作指令集 */
#define WRITE            0x02     /* 写数据到内存指令 */
#define WRSR             0x01     /* 写状态寄存器指令 */
#define WREN             0x06     /* 写使能指令（解除写保护）*/
#define READ             0x03     /* 从内存读取数据指令 */
#define RDSR             0x05     /* 读状态寄存器指令 */
#define RDID             0x9F     /* 读芯片ID指令 */
#define SE               0x20     /* 扇区擦除指令 */
#define BE               0xC7     /* 整片擦除指令 */

/* 状态标志和占位字节 */
#define WIP_FLAG         0x01     /* 写操作进行中标志位（WIP） */
#define DUMMY_BYTE       0xA5     /* 哑字节（用于填充传输） */


/*!
    \brief      初始化 SPI1 GPIO 和参数
    \param[in]  无
    \param[out] 无
    \retval     无
*/


void spi_flash_init(void)
{
    spi_parameter_struct spi_init_struct;

    // 使能 GPIOB 和 SPI1 的时钟
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_SPI1);
	
    /* 配置 SPI1 功能引脚（PB13-SCK, PB14-MISO, PB15-MOSI）*/
    // 设置复用功能（AF5）
    gpio_af_set(GPIOB, GPIO_AF_5, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
    // 配置为复用功能模式，无上下拉
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
    // 输出配置：推挽输出，25MHz速度
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);

    /* 配置 SPI Flash 片选引脚（PB12）*/
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_12);
    // 输出配置：推挽输出，50MHz高速
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);

    /* 初始状态：片选无效（高电平）*/
    SPI_FLASH_CS_HIGH();

    /* 配置 SPI1 参数 */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX; // 全双工模式
    spi_init_struct.device_mode          = SPI_MASTER;               // 主机模式
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;       // 8位数据帧
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;   // 时钟极性：低电平空闲，第1个边沿采样
    spi_init_struct.nss                  = SPI_NSS_SOFT;             // 软件控制片选
    spi_init_struct.prescale             = SPI_PSC_8;                // 8分频（假设系统时钟108MHz，SPI时钟13.5MHz）
    spi_init_struct.endian               = SPI_ENDIAN_MSB;           // 高位在前
    spi_init(SPI1, &spi_init_struct);    // 应用配置到SPI1

    /* 使能 SPI1 外设 */
    spi_enable(SPI1);
}


/*!
    \brief      擦除指定 Flash 扇区
    \param[in]  sector_addr: 要擦除的扇区地址
    \param[out] 无
    \retval     无
*/
void spi_flash_sector_erase(uint32_t sector_addr)
{
    /* 步骤1：发送写使能指令（解除写保护）*/
    spi_flash_write_enable();

    /* 步骤2：执行扇区擦除 */
    SPI_FLASH_CS_LOW();  // 拉低片选，开始通信
    
    // 发送扇区擦除指令（0x20）
    spi_flash_send_byte(SE);
    // 发送24位地址（分3个字节）
    spi_flash_send_byte((sector_addr & 0xFF0000) >> 16); // 地址高字节
    spi_flash_send_byte((sector_addr & 0xFF00) >> 8);    // 地址中字节
    spi_flash_send_byte(sector_addr & 0xFF);             // 地址低字节
    
    SPI_FLASH_CS_HIGH(); // 拉高片选，结束通信

    /* 步骤3：等待擦除操作完成 */
    spi_flash_wait_for_write_end();
}



/*!
    \brief       擦除 Flash 中指定长度的数据
    \param[in]  sector_addr: 擦除操作的起始地址（24位地址）
    \param[out] num_byte_to_erase: 需要擦除的字节长度
    \retval     none
*/
void spi_flash_buffer_erase(uint32_t sector_addr,  uint32_t num_byte_to_erase)
{
	uint8_t buffer_data[SPI_FLASH_SECTOR_SIZE] = {0};
	uint8_t buffer_data1[SPI_FLASH_SECTOR_SIZE] = {0};
	uint8_t num_of_sector = 0, num_of_single = 0, addr = 0, count = 0, temp = 0;


    addr          = sector_addr % SPI_FLASH_SECTOR_SIZE;		//扇区内地址
    count         = SPI_FLASH_PAGE_SIZE - addr;					//页内剩下可以写的空间长度
    num_of_sector = num_byte_to_erase / SPI_FLASH_SECTOR_SIZE;	//要擦除多少个满扇区空间
    num_of_single = num_byte_to_erase % SPI_FLASH_SECTOR_SIZE;	//剩下要擦除不满一个扇区的字节数
	
	/* 待擦除的地址是扇区对齐的 */
    if(0 == addr)
	{

		while(num_of_sector-- )		//擦除整数个扇区
		{
			spi_flash_sector_erase( sector_addr );
			sector_addr += SPI_FLASH_PAGE_SIZE;
		}
		if(0 != num_of_single)		//擦除小数个扇区
		{
			spi_flash_buffer_read(buffer_data, sector_addr+num_of_single, SPI_FLASH_SECTOR_SIZE-num_of_single);			//先读出扇区后部分内容
			spi_flash_sector_erase( sector_addr );		//再擦
			spi_flash_buffer_write(buffer_data, sector_addr+num_of_single, SPI_FLASH_SECTOR_SIZE-num_of_single);		//再写回扇区后部分内容	
		}
    }
	else/* 待擦除的地址是非扇区对齐的 */
	{
        if(num_byte_to_erase < count)
		{
			spi_flash_buffer_read(buffer_data, num_of_sector*SPI_FLASH_SECTOR_SIZE, addr);																	//先读出扇区前部分内容
			spi_flash_buffer_read(buffer_data1, num_of_sector*SPI_FLASH_SECTOR_SIZE+addr+num_byte_to_erase, SPI_FLASH_SECTOR_SIZE-(addr)-num_byte_to_erase);//  读出扇区后部分内容
			spi_flash_sector_erase( sector_addr );		//再擦
			spi_flash_buffer_write(buffer_data, num_of_sector*SPI_FLASH_SECTOR_SIZE, addr);																	//再写回扇区前部分内容
			spi_flash_buffer_write(buffer_data1, num_of_sector*SPI_FLASH_SECTOR_SIZE+addr+num_byte_to_erase, SPI_FLASH_SECTOR_SIZE-(addr)-num_byte_to_erase);//再写回扇区前部分内容
		}
		else
		{
			spi_flash_buffer_read(buffer_data, num_of_sector*SPI_FLASH_SECTOR_SIZE, addr);	//先读出扇区前部分内容
			spi_flash_sector_erase( sector_addr );		//再擦
			spi_flash_buffer_write(buffer_data, num_of_sector*SPI_FLASH_SECTOR_SIZE, addr);		//再写回扇区前部分内容
			
			//现在擦除地址又变得扇区对齐了
			num_byte_to_erase -= addr;
			num_of_sector = num_byte_to_erase / SPI_FLASH_SECTOR_SIZE;//要擦除多少个满扇区空间
			num_of_single = num_byte_to_erase % SPI_FLASH_SECTOR_SIZE;//剩下要擦除不满一个扇区的字节数
			sector_addr += count;
			
			while(num_of_sector-- )		//擦除整数个扇区
			{
				spi_flash_sector_erase( sector_addr );
				sector_addr += SPI_FLASH_PAGE_SIZE;
			}
			if(0 != num_of_single)		//擦除小数个扇区
			{
				spi_flash_buffer_read(buffer_data, sector_addr+num_of_single, SPI_FLASH_SECTOR_SIZE-num_of_single);			//先读出来
				spi_flash_sector_erase( sector_addr );		//再擦
				spi_flash_buffer_write(buffer_data, sector_addr+num_of_single, SPI_FLASH_SECTOR_SIZE-num_of_single);	//再写回去	
			}	
		}
    }
}



/*!
    \brief      擦除整个 Flash 芯片
    \param[in]  无
    \param[out] 无
    \retval     无

    \note       整片擦除操作会将所有存储单元重置为 0xFF
                此操作耗时较长（通常数秒），期间不可断电
*/
void spi_flash_bulk_erase(void)
{
    /* 步骤1：发送写使能指令（解除写保护）*/
    spi_flash_write_enable();

    /* 步骤2：执行整片擦除 */
    SPI_FLASH_CS_LOW();      // 拉低片选，开始通信
    spi_flash_send_byte(BE); // 发送整片擦除指令 (0xC7)
    SPI_FLASH_CS_HIGH();     // 拉高片选，结束通信

    /* 步骤3：等待擦除操作完成 */
    spi_flash_wait_for_write_end();
}


/*!
    \brief      向 Flash 写入一页数据
    \param[in]  pbuffer: 指向要写入数据的缓冲区
    \param[in]  write_addr: Flash 内部写入地址（24位）
    \param[in]  num_byte_to_write: 要写入的字节数
    \param[out] 无
    \retval     无

    \warning    重要限制：
                1. 不能跨页写入（页大小通常256字节）
                2. 写入前必须确保目标区域已被擦除（全FF）
                3. 写入地址必须按页对齐
*/
void spi_flash_page_write(uint8_t* pbuffer, uint32_t write_addr, uint16_t num_byte_to_write)
{
    /* 步骤1：发送写使能指令 */
    spi_flash_write_enable();

    /* 步骤2：开始数据传输 */
    SPI_FLASH_CS_LOW();  // 拉低片选，开始通信
    
    // 发送写指令 (0x02)
    spi_flash_send_byte(WRITE);
    
    // 发送24位地址（分3字节）
    spi_flash_send_byte((write_addr & 0xFF0000) >> 16); // 地址高字节
    spi_flash_send_byte((write_addr & 0xFF00) >> 8);    // 地址中字节
    spi_flash_send_byte(write_addr & 0xFF);             // 地址低字节

    /* 步骤3：循环写入数据 */
    while(num_byte_to_write-- > 0) 
    {
        spi_flash_send_byte(*pbuffer); // 发送当前字节
        pbuffer++;                     // 指向下一个字节
    }

    /* 步骤4：结束传输 */
    SPI_FLASH_CS_HIGH(); // 拉高片选，结束通信
    
    /* 步骤5：等待写入完成 */
    spi_flash_wait_for_write_end();
}




/*!
    \brief      向 Flash 写入任意长度的数据块
    \param[in]  pbuffer: 指向待写入数据缓冲区的指针
    \param[in]  write_addr: Flash 内部写入起始地址（24位）
    \param[in]  num_byte_to_write: 要写入的字节总数
    \param[out] 无
    \retval     无
*/
void spi_flash_buffer_write(uint8_t* pbuffer, uint32_t write_addr, uint32_t num_byte_to_write)
{
    // 定义计算变量
    uint8_t num_of_page = 0,   // 完整页数
            num_of_single = 0, // 剩余单字节数
            addr = 0,          // 页内偏移地址
            count = 0;         // 当前页剩余空间
    
    // 计算页内偏移地址
    addr = write_addr % SPI_FLASH_PAGE_SIZE;
    // 计算当前页剩余空间
    count = SPI_FLASH_PAGE_SIZE - addr;
    // 计算完整页数
    num_of_page = num_byte_to_write / SPI_FLASH_PAGE_SIZE;
    // 计算剩余字节数
    num_of_single = num_byte_to_write % SPI_FLASH_PAGE_SIZE;

	
	
       /* 情况1：起始地址页对齐 */
    if(0 == addr)
    {
        // 循环写入完整页
        while(num_of_page-- )
        {
            spi_flash_page_write(pbuffer, write_addr, SPI_FLASH_PAGE_SIZE);
            write_addr += SPI_FLASH_PAGE_SIZE;  // 地址递增
            pbuffer += SPI_FLASH_PAGE_SIZE;     // 缓冲区指针递增
        }
        
        // 写入剩余字节
        if(0 != num_of_single)
            spi_flash_page_write(pbuffer, write_addr, num_of_single);
    }
    /* 情况2：起始地址非页对齐 */
    else
    {
        // 情况2.1：数据长度小于当前页剩余空间
        if(num_byte_to_write < count)
        {
            spi_flash_page_write(pbuffer, write_addr, num_byte_to_write);
        }
        // 情况2.2：数据长度大于等于当前页剩余空间
        else
        {
            // 步骤1：写满当前页剩余空间
            spi_flash_page_write(pbuffer, write_addr, count);
            
            // 更新地址和缓冲区指针
            write_addr += count;
            pbuffer += count;
            
            // 重新计算完整页数和剩余字节数
            num_of_page   = (num_byte_to_write - count) / SPI_FLASH_PAGE_SIZE;
            num_of_single = (num_byte_to_write - count) % SPI_FLASH_PAGE_SIZE;
            
            // 循环写入完整页
            while(num_of_page-- )
            {
                spi_flash_page_write(pbuffer, write_addr, SPI_FLASH_PAGE_SIZE);
                write_addr += SPI_FLASH_PAGE_SIZE;
                pbuffer += SPI_FLASH_PAGE_SIZE;
            }
            
            // 写入剩余字节
            if(0 != num_of_single)
                spi_flash_page_write(pbuffer, write_addr, num_of_single);	
        }
    }
}

/*!
    \brief      从 Flash 读取一块数据
    \param[in]  pbuffer: 接收数据缓冲区指针
    \param[in]  read_addr: Flash 读取起始地址
    \param[in]  num_byte_to_read: 读取字节总数
    \param[out] 无
    \retval     无
*/


void spi_flash_buffer_read(uint8_t* pbuffer, uint32_t read_addr, uint16_t num_byte_to_read)
{
    /* 选中 Flash 芯片 */
    SPI_FLASH_CS_LOW();

    /* 发送读指令 (0x03) */
    spi_flash_send_byte(READ);

    /* 发送 24 位地址 (3字节) */
    spi_flash_send_byte((read_addr & 0xFF0000) >> 16);  // 地址高字节
    spi_flash_send_byte((read_addr & 0xFF00) >> 8);     // 地址中字节
    spi_flash_send_byte(read_addr & 0xFF);              // 地址低字节

    /* 循环读取数据 */
    while(num_byte_to_read--){
        /* 读取一个字节 (发送哑元字节获取数据) */
        *pbuffer = spi_flash_send_byte(DUMMY_BYTE);
        /* 指针递增 */
        pbuffer++;
    }

    /* 取消选中 Flash 芯片 */
    SPI_FLASH_CS_HIGH();
}

/*!
    \brief      读取 Flash 芯片 ID
    \param[in]  无
    \param[out] 无
    \retval     32位ID值 (实际使用24位)

    \details    ID组成：
                字节1: 制造商ID
                字节2: 设备类型
                字节3: 设备容量
*/
uint32_t spi_flash_read_id(void)
{
    uint32_t temp = 0;
    uint8_t temp0 = 0, temp1 = 0, temp2 = 0;

    /* 选中 Flash 芯片 */
    SPI_FLASH_CS_LOW();

    /* 发送读ID指令 (0x9F) */
    spi_flash_send_byte(RDID);

    /* 连续读取3字节ID */
    temp0 = spi_flash_send_byte(DUMMY_BYTE);  // 制造商ID
    temp1 = spi_flash_send_byte(DUMMY_BYTE);  // 设备类型
    temp2 = spi_flash_send_byte(DUMMY_BYTE);  // 设备容量

    /* 取消选中 Flash 芯片 */
    SPI_FLASH_CS_HIGH();

    /* 组合3字节为32位值 */
    temp = (temp0 << 16) | (temp1 << 8) | temp2;

    return temp;
}

/*!
    \brief      启动连续读取序列
    \param[in]  read_addr: 读取起始地址
    \param[out] 无
    \retval     无

    \note       用于高效连续读取，后续使用 spi_flash_read_byte 获取数据
                 必须保持片选低电平直到读取完成
*/
void spi_flash_start_read_sequence(uint32_t read_addr)
{
    /* 选中 Flash 芯片 */
    SPI_FLASH_CS_LOW();

    /* 发送读指令 (0x03) */
    spi_flash_send_byte(READ);

    /* 发送 24 位地址 */
    spi_flash_send_byte((read_addr & 0xFF0000) >> 16);  // 高字节
    spi_flash_send_byte((read_addr & 0xFF00) >> 8);     // 中字节
    spi_flash_send_byte(read_addr & 0xFF);              // 低字节
}

/*!
    \brief      从连续读取序列中读取一个字节
    \param[in]  无
    \param[out] 无
    \retval     读取到的字节

    \note       必须在 spi_flash_start_read_sequence 后使用
                 每次读取后地址自动递增
*/
uint8_t spi_flash_read_byte(void)
{
    /* 发送哑元字节获取数据 */
    return spi_flash_send_byte(DUMMY_BYTE);
}


/*!
    \brief      SPI 发送一个字节并接收返回字节
    \param[in]  byte: 要发送的字节
    \param[out] 无
    \retval     接收到的字节

    \details    全双工 SPI 通信实现
*/
uint8_t spi_flash_send_byte(uint8_t byte)
{
    /* 等待发送缓冲区空 */
    while (RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));

    /* 发送字节 */
    spi_i2s_data_transmit(SPI1, byte);

    /* 等待接收缓冲区非空 */
    while(RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE));

    /* 返回接收到的字节 */
    return spi_i2s_data_receive(SPI1);
}


/*!
    \brief      SPI 发送半字并接收返回半字
    \param[in]  half_word: 要发送的半字(16位)
    \param[out] 无
    \retval     接收到的半字
*/
uint16_t spi_flash_send_halfword(uint16_t half_word)
{
    /* 等待发送缓冲区空 */
    while(RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));

    /* 发送半字 */
    spi_i2s_data_transmit(SPI1, half_word);

    /* 等待接收缓冲区非空 */
    while(RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE));

    /* 返回接收到的半字 */
    return spi_i2s_data_receive(SPI1);
}

/*!
    \brief      发送写使能命令
    \param[in]  无
    \param[out] 无
    \retval     无

    \note       在执行任何写/擦除操作前必须调用
*/
void spi_flash_write_enable(void)
{
    /* 选中 Flash 芯片 */
    SPI_FLASH_CS_LOW();

    /* 发送写使能指令 (0x06) */
    spi_flash_send_byte(WREN);

    /* 取消选中 Flash 芯片 */
    SPI_FLASH_CS_HIGH();
}

/*!
    \brief      等待写操作完成
    \param[in]  无
    \param[out] 无
    \retval     无

    \details    通过轮询状态寄存器的WIP位(bit0)
*/
void spi_flash_wait_for_write_end(void)
{
    uint8_t flash_status = 0;

    /* 选中 Flash 芯片 */
    SPI_FLASH_CS_LOW();

    /* 发送读状态寄存器指令 (0x05) */
    spi_flash_send_byte(RDSR);

    /* 轮询WIP位(bit0)直到为0 */
    do {
        /* 发送哑元字节获取状态 */
        flash_status = spi_flash_send_byte(DUMMY_BYTE);
    } while((flash_status & WIP_FLAG) == SET); // WIP_FLAG = 0x01

    /* 取消选中 Flash 芯片 */
    SPI_FLASH_CS_HIGH();
}
