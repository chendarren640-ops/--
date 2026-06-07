/*!
    \file  sdcard.c
    \brief SD卡驱动
    
    \version 2016-08-15, V1.0.0, firmware for GD32F4xx
    \version 2018-12-12, V2.0.0, firmware for GD32F4xx
*/

/*
    Copyright (c) 2018, GigaDevice Semiconductor Inc.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "sdcard.h"
#include "gd32f4xx_sdio.h"
#include "gd32f4xx_dma.h"
#include <stddef.h>

/* R1响应中的卡状态位定义（位31～8等） */
#define SD_R1_OUT_OF_RANGE                  BIT(31)                   /* 命令参数超出允许范围 */
#define SD_R1_ADDRESS_ERROR                 BIT(30)                   /* 地址未对齐或与块长度不匹配 */
#define SD_R1_BLOCK_LEN_ERROR               BIT(29)                   /* 传输块长度不被允许 */
#define SD_R1_ERASE_SEQ_ERROR               BIT(28)                   /* 擦除命令序列错误 */
#define SD_R1_ERASE_PARAM                   BIT(27)                   /* 擦除写块选择无效 */
#define SD_R1_WP_VIOLATION                  BIT(26)                   /* 试图写保护块或临时/永久写保护卡 */
#define SD_R1_CARD_IS_LOCKED                BIT(25)                   /* 卡已被主机锁定 */
#define SD_R1_LOCK_UNLOCK_FAILED            BIT(24)                   /* 锁定/解锁命令序列或密码错误 */
#define SD_R1_COM_CRC_ERROR                 BIT(23)                   /* 前一个命令的CRC校验失败 */
#define SD_R1_ILLEGAL_COMMAND               BIT(22)                   /* 命令对当前卡状态不合法 */
#define SD_R1_CARD_ECC_FAILED               BIT(21)                   /* 卡内部ECC应用但数据校正失败 */
#define SD_R1_CC_ERROR                      BIT(20)                   /* 内部卡控制器错误 */
#define SD_R1_GENERAL_UNKNOWN_ERROR         BIT(19)                   /* 操作期间发生一般或未知错误 */
#define SD_R1_CSD_OVERWRITE                 BIT(16)                   /* CSD只读区不匹配或试图反转复制或永久WP位 */
#define SD_R1_WP_ERASE_SKIP                 BIT(15)                   /* 部分地址空间未被擦除 */
#define SD_R1_CARD_ECC_DISABLED             BIT(14)                   /* 命令执行时未使用内部ECC */
#define SD_R1_ERASE_RESET                   BIT(13)                   /* 擦除序列在执行前被清除 */
#define SD_R1_READY_FOR_DATA                BIT(8)                    /* 对应总线上的缓冲区空信号 */
#define SD_R1_APP_CMD                       BIT(5)                    /* 卡将期望ACMD命令 */
#define SD_R1_AKE_SEQ_ERROR                 BIT(3)                    /* 认证过程序列错误 */
#define SD_R1_ERROR_BITS                    (uint32_t)0xFDF9E008      /* 所有R1错误位掩码 */

/* R6响应中的卡状态位定义 */
#define SD_R6_COM_CRC_ERROR                 BIT(15)                   /* 前一个命令的CRC校验失败 */
#define SD_R6_ILLEGAL_COMMAND               BIT(14)                   /* 命令不合法 */
#define SD_R6_GENERAL_UNKNOWN_ERROR         BIT(13)                   /* 一般/未知错误 */

/* 卡状态编码 */
#define SD_CARDSTATE_IDLE                   ((uint8_t)0x00)           /* 空闲状态 */
#define SD_CARDSTATE_READY                  ((uint8_t)0x01)           /* 就绪状态 */
#define SD_CARDSTATE_IDENTIFICAT            ((uint8_t)0x02)           /* 识别状态 */
#define SD_CARDSTATE_STANDBY                ((uint8_t)0x03)           /* 待机状态 */
#define SD_CARDSTATE_TRANSFER               ((uint8_t)0x04)           /* 传输状态 */
#define SD_CARDSTATE_DATA                   ((uint8_t)0x05)           /* 数据发送状态 */
#define SD_CARDSTATE_RECEIVING              ((uint8_t)0x06)           /* 接收状态 */
#define SD_CARDSTATE_PROGRAMMING            ((uint8_t)0x07)           /* 编程状态 */
#define SD_CARDSTATE_DISCONNECT             ((uint8_t)0x08)           /* 断开状态 */
#define SD_CARDSTATE_LOCKED                 ((uint32_t)0x02000000)    /* 锁定状态 */

#define SD_CHECK_PATTERN                    ((uint32_t)0x000001AA)    /* CMD8检测模式 */
#define SD_VOLTAGE_WINDOW                   ((uint32_t)0x80100000)    /* ACMD41主机3.3V请求 */

/* ACMD41参数（电压验证） */
#define SD_HIGH_CAPACITY                    ((uint32_t)0x40000000)    /* 高容量SD存储卡 */
#define SD_STD_CAPACITY                     ((uint32_t)0x00000000)    /* 标准容量SD存储卡 */

/* SD总线宽度，通过SCR寄存器检查 */
#define SD_BUS_WIDTH_4BIT                   ((uint32_t)0x00040000)    /* 4位总线模式 */
#define SD_BUS_WIDTH_1BIT                   ((uint32_t)0x00010000)    /* 1位总线模式 */

/* SCR寄存器位段掩码 */
#define SD_MASK_0_7BITS                     ((uint32_t)0x000000FF)    /* [7:0]掩码 */
#define SD_MASK_8_15BITS                    ((uint32_t)0x0000FF00)    /* [15:8]掩码 */
#define SD_MASK_16_23BITS                   ((uint32_t)0x00FF0000)    /* [23:16]掩码 */
#define SD_MASK_24_31BITS                   ((uint32_t)0xFF000000)    /* [31:24]掩码 */

#define SDIO_FIFO_ADDR                      ((uint32_t)0x40012C80)    /* SDIO_FIFO地址 */
#define SD_FIFOHALF_WORDS                   ((uint32_t)0x00000008)    /* FIFO半满/半空字数 */
#define SD_FIFOHALF_BYTES                   ((uint32_t)0x00000020)    /* FIFO半满/半空字节数 */

#define SD_DATATIMEOUT                      ((uint32_t)0xFFFFFFFF)    /* 数据超时计数 */
#define SD_MAX_VOLT_VALIDATION              ((uint32_t)0x0000FFFF)    /* 电压验证最大尝试次数 */
#define SD_MAX_DATA_LENGTH                  ((uint32_t)0x01FFFFFF)    /* 数据最大长度 */
#define SD_ALLZERO                          ((uint32_t)0x00000000)    /* 全零 */
#define SD_RCA_SHIFT                        ((uint8_t)0x10)           /* RCA左移位宽 */
#define SD_CLK_DIV_INIT                     ((uint16_t)0x0076)        /* 初始化阶段SD时钟分频 */
#define SD_CLK_DIV_TRANS                    ((uint16_t)0x0000)        /* 传输阶段SD时钟分频 */

#define SDIO_MASK_INTC_FLAGS                ((uint32_t)0x00C007FF)    /* SDIO_INTC中断标志掩码 */

uint32_t sd_scr[2] = {0,0};                                           /* SCR寄存器内容 */

static sdio_card_type_enum cardtype = SDIO_STD_CAPACITY_SD_CARD_V1_1; /* SD卡类型 */
static uint32_t sd_csd[4] = {0,0,0,0};                                /* CSD寄存器内容 */
static uint32_t sd_cid[4] = {0,0,0,0};                                /* CID寄存器内容 */
static uint16_t sd_rca = 0;                                           /* SD卡的RCA */
static uint32_t transmode = SD_POLLING_MODE;
static uint32_t totalnumber_bytes = 0, stopcondition = 0;
static __IO sd_error_enum transerror = SD_OK;
static __IO uint32_t transend = 0, number_bytes = 0;

/* 检查命令发送是否出错 */
static sd_error_enum cmdsent_error_check(void);
/* 检查R1响应是否有错误 */
static sd_error_enum r1_error_check(uint8_t cmdindex);
/* 根据R1响应内容判断错误类型 */
static sd_error_enum r1_error_type_check(uint32_t resp);
/* 检查R2响应是否有错误 */
static sd_error_enum r2_error_check(void);
/* 检查R3响应是否有错误 */
static sd_error_enum r3_error_check(void);
/* 检查R6响应是否有错误并提取RCA */
static sd_error_enum r6_error_check(uint8_t cmdindex, uint16_t *prca);
/* 检查R7响应是否有错误 */
static sd_error_enum r7_error_check(void);

/* 获取卡当前状态 */
static sd_error_enum sd_card_state_get(uint8_t *pcardstate);
/* 配置总线宽度模式 */
static sd_error_enum sd_bus_width_config(uint32_t buswidth);
/* 获取卡的SCR寄存器内容 */
static sd_error_enum sd_scr_get(uint16_t rca, uint32_t *pscr);
/* 根据字节数获取SDIO数据块大小编码 */
static uint32_t sd_datablocksize_get(uint16_t bytesnumber);

/* 配置SDIO接口GPIO */
static void gpio_config(void);
/* 配置SDIO和DMA的时钟 */
static void rcu_config(void);
/* 配置DMA用于SDIO发送请求 */
static void dma_transfer_config(uint32_t *srcbuf, uint32_t bufsize);
/* 配置DMA用于SDIO接收请求 */
static void dma_receive_config(uint32_t *dstbuf, uint32_t bufsize);

/*!
    \brief      初始化SD卡并使其进入待机状态
    \param[in]  无
    \param[out] 无
    \retval     sd_error_enum 状态码
*/
sd_error_enum sd_init(void)
{
    sd_error_enum status = SD_OK;
    /* 配置RCU和GPIO，复位SDIO外设 */
    rcu_config();
    gpio_config();
    sdio_deinit();
    
    /* 配置时钟和工作电压 */
    status = sd_power_on();
    if(SD_OK != status){
        return status;
    }
    
    /* 初始化卡并获取CID和CSD */
    status = sd_card_init();
    if(SD_OK != status){
        return status;
    }
    
    /* 配置SDIO外设：上升沿采样，不分频，不省电，传输分频为0 */
    sdio_clock_config(SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKBYPASS_DISABLE, SDIO_CLOCKPWRSAVE_DISABLE, SD_CLK_DIV_TRANS);
    sdio_bus_mode_set(SDIO_BUSMODE_1BIT);
    sdio_hardware_clock_disable();
    
    return status;
}

/*!
    \brief      初始化卡并获取CID和CSD
    \param[in]  无
    \param[out] 无
    \retval     sd_error_enum 状态码
*/
sd_error_enum sd_card_init(void)
{
    sd_error_enum status = SD_OK;
    uint16_t temp_rca = 0x01;
    
    if(SDIO_POWER_OFF == sdio_power_state_get()){
        status = SD_OPERATION_IMPROPER;
        return status;
    }
    
    /* 卡不是纯I/O卡 */
    if(SDIO_SECURE_DIGITAL_IO_CARD != cardtype){
        /* 发送CMD2(SD_CMD_ALL_SEND_CID)获取CID */
        sdio_command_response_config(SD_CMD_ALL_SEND_CID, (uint32_t)0x0, SDIO_RESPONSETYPE_LONG);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        /* 检查是否出错 */
        status = r2_error_check();
        if(SD_OK != status){
            return status;
        }
        
        /* 存储CID */
        sd_cid[0] = sdio_response_get(SDIO_RESPONSE0);
        sd_cid[1] = sdio_response_get(SDIO_RESPONSE1);
        sd_cid[2] = sdio_response_get(SDIO_RESPONSE2);
        sd_cid[3] = sdio_response_get(SDIO_RESPONSE3);
    }
    
    /* 卡是SD存储卡或I/O复合卡 */
    if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == cardtype) || 
        (SDIO_HIGH_CAPACITY_SD_CARD == cardtype) || (SDIO_SECURE_DIGITAL_IO_COMBO_CARD == cardtype)){
        /* 发送CMD3(SEND_RELATIVE_ADDR)获取RCA */
        sdio_command_response_config(SD_CMD_SEND_RELATIVE_ADDR, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        /* 检查错误并保存RCA */
        status = r6_error_check(SD_CMD_SEND_RELATIVE_ADDR, &temp_rca);
        if(SD_OK != status){
            return status;
        }
    }
    
    if(SDIO_SECURE_DIGITAL_IO_CARD != cardtype){
        /* 卡不是纯I/O卡 */
        sd_rca = temp_rca;
        
        /* 发送CMD9(SEND_CSD)获取CSD */
        sdio_command_response_config(SD_CMD_SEND_CSD, (uint32_t)(temp_rca << SD_RCA_SHIFT), SDIO_RESPONSETYPE_LONG);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        /* 检查是否出错 */
        status = r2_error_check();
        if(SD_OK != status){
            return status;
        }
        
        /* 存储CSD */
        sd_csd[0] = sdio_response_get(SDIO_RESPONSE0);
        sd_csd[1] = sdio_response_get(SDIO_RESPONSE1);
        sd_csd[2] = sdio_response_get(SDIO_RESPONSE2);
        sd_csd[3] = sdio_response_get(SDIO_RESPONSE3);
    }
    return status;
}

/*!
    \brief      配置时钟和工作电压，并获取卡类型
    \param[in]  无
    \param[out] 无
    \retval     sd_error_enum 状态码
*/
sd_error_enum sd_power_on(void)
{
    sd_error_enum status = SD_OK;
    uint32_t sdcardtype = SD_STD_CAPACITY, response = 0, count = 0;
    uint8_t busyflag = 0;
    
    /* 配置SDIO外设：上升沿，不分频，不省电，初始化分频 */
    sdio_clock_config(SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKBYPASS_DISABLE, SDIO_CLOCKPWRSAVE_DISABLE, SD_CLK_DIV_INIT);
    sdio_bus_mode_set(SDIO_BUSMODE_1BIT);
    sdio_hardware_clock_disable();
    sdio_power_state_set(SDIO_POWER_ON);
    /* 使能SDIO_CLK时钟输出 */
    sdio_clock_enable();
    
    /* 发送CMD0(GO_IDLE_STATE)复位卡 */
    sdio_command_response_config(SD_CMD_GO_IDLE_STATE, (uint32_t)0x0, SDIO_RESPONSETYPE_NO);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    /* 使能命令状态机 */
    sdio_csm_enable();
    
    /* 检查命令发送是否出错 */
    status = cmdsent_error_check();
    if(SD_OK != status){
        return status;
    }
    
    /* 发送CMD8(SEND_IF_COND)获取SD存储卡接口条件 */
    sdio_command_response_config(SD_CMD_SEND_IF_COND, SD_CHECK_PATTERN, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    
    if(SD_OK == r7_error_check()){
        /* SD卡2.0 */
        cardtype = SDIO_STD_CAPACITY_SD_CARD_V2_0;
        sdcardtype = SD_HIGH_CAPACITY;
    }
    
    /* 发送CMD55(APP_CMD)指示下一个命令是应用特定命令 */
    sdio_command_response_config(SD_CMD_APP_CMD, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    
    if(SD_OK == r1_error_check(SD_CMD_APP_CMD)){
        /* SD存储卡 */
        while((!busyflag) && (count < SD_MAX_VOLT_VALIDATION)){
            /* 发送CMD55 */
            sdio_command_response_config(SD_CMD_APP_CMD, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO_WAITTYPE_NO);
            sdio_csm_enable();
            /* 检查错误 */
            status = r1_error_check(SD_CMD_APP_CMD);
            if(SD_OK != status){
                return status;
            }
            
            /* 发送ACMD41(SD_SEND_OP_COND)获取HCS和OCR */
            sdio_command_response_config(SD_APPCMD_SD_SEND_OP_COND, (SD_VOLTAGE_WINDOW | sdcardtype), SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO_WAITTYPE_NO);
            sdio_csm_enable();
            /* 检查错误 */
            status = r3_error_check();
            if(SD_OK != status){
                return status;
            }
            /* 获取响应并检查卡上电状态位(busy) */
            response = sdio_response_get(SDIO_RESPONSE0);
            busyflag = (uint8_t)((response >> 31)&(uint32_t)0x01);
            ++count;
        }
        if(count >= SD_MAX_VOLT_VALIDATION){
            status = SD_VOLTRANGE_INVALID;
            return status;
        }
        if(response &= SD_HIGH_CAPACITY){
            /* SDHC卡 */
            cardtype = SDIO_HIGH_CAPACITY_SD_CARD;
        }
    }
    return status;
}

/*!
    \brief      关闭SDIO电源
    \param[in]  无
    \param[out] 无
    \retval     sd_error_enum
*/
sd_error_enum sd_power_off(void)
{
    sd_error_enum status = SD_OK;
    sdio_power_state_set(SDIO_POWER_OFF);
    return status;
}

/*!
    \brief      配置总线模式（1位/4位）
    \param[in]  busmode: 总线模式
      \arg        SDIO_BUSMODE_1BIT: 1位模式
      \arg        SDIO_BUSMODE_4BIT: 4位模式
      \arg        SDIO_BUSMODE_8BIT: 8位模式(仅MMC)
    \param[out] 无
    \retval     sd_error_enum
*/
sd_error_enum sd_bus_mode_config(uint32_t busmode)
{
    sd_error_enum status = SD_OK;
    if(SDIO_MULTIMEDIA_CARD == cardtype){
        /* MMC卡不支持此功能 */
        status = SD_FUNCTION_UNSUPPORTED;
        return status;
    }else if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == cardtype) || 
             (SDIO_HIGH_CAPACITY_SD_CARD == cardtype)){
        if(SDIO_BUSMODE_8BIT == busmode){
            /* 8位总线模式不支持 */
            status = SD_FUNCTION_UNSUPPORTED;
            return status;
        }else if(SDIO_BUSMODE_4BIT == busmode){
            /* 配置4位总线 */
            status = sd_bus_width_config(SD_BUS_WIDTH_4BIT);
            if(SD_OK == status){
                sdio_clock_config(SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKBYPASS_DISABLE,
                                    SDIO_CLOCKPWRSAVE_DISABLE, SD_CLK_DIV_TRANS);
                sdio_bus_mode_set(busmode);
                sdio_hardware_clock_disable();
            }
        }else if(SDIO_BUSMODE_1BIT == busmode){
            /* 配置1位总线 */
            status = sd_bus_width_config(SD_BUS_WIDTH_1BIT);
            if(SD_OK == status){
                sdio_clock_config(SDIO_SDIOCLKEDGE_RISING, SDIO_CLOCKBYPASS_DISABLE, 
                                    SDIO_CLOCKPWRSAVE_DISABLE, SD_CLK_DIV_TRANS);
                sdio_bus_mode_set(busmode);
                sdio_hardware_clock_disable();
            }
        }else{
            status = SD_PARAMETER_INVALID;
        }
    }
    return status;
}

/*!
    \brief      配置数据传输模式（DMA或轮询）
    \param[in]  txmode: 传输模式
      \arg        SD_DMA_MODE: DMA模式
      \arg        SD_POLLING_MODE: 轮询模式
    \param[out] 无
    \retval     sd_error_enum
*/
sd_error_enum sd_transfer_mode_config(uint32_t txmode)
{
    sd_error_enum status = SD_OK;
    /* 设置传输模式 */
    if((SD_DMA_MODE == txmode) || (SD_POLLING_MODE == txmode)){
        transmode = txmode;
    }else{
        status = SD_PARAMETER_INVALID;
    }
    return status;
}

/*!
    \brief      从卡的指定地址读取一个数据块到缓冲区
    \param[out] preadbuffer: 存放读取数据的指针
    \param[in]  readaddr: 读取地址（SDHC卡以块为单位）
    \param[in]  blocksize: 数据块大小（字节）
    \retval     sd_error_enum
*/
sd_error_enum sd_block_read(uint32_t *preadbuffer, uint32_t readaddr, uint16_t blocksize)
{
    /* 初始化变量 */
    sd_error_enum status = SD_OK;
    uint32_t count = 0, align = 0, datablksize = SDIO_DATABLOCKSIZE_1BYTE, *ptempbuff = preadbuffer;
    __IO uint32_t timeout = 0;
    
    if(NULL == preadbuffer){
        status = SD_PARAMETER_INVALID;
        return status;
    }
    
    transerror = SD_OK;
    transend = 0;
    totalnumber_bytes = 0;
    /* 清除所有DSM配置 */
    sdio_data_config(0, 0, SDIO_DATABLOCKSIZE_1BYTE);
    sdio_data_transfer_config(SDIO_TRANSDIRECTION_TOCARD, SDIO_TRANSMODE_BLOCK);
    sdio_dsm_disable();
    sdio_dma_disable();
    
    /* 检查卡是否锁定 */
    if(sdio_response_get(SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED){
        status = SD_LOCK_UNLOCK_FAILED;
        return status;
    }
    
    /* SDHC卡块大小固定为512字节，地址以块号给出 */
    if (SDIO_HIGH_CAPACITY_SD_CARD == cardtype)
    {
        blocksize = 512;
        readaddr /= 512;
    }
    
    align = blocksize & (blocksize - 1);
    if((blocksize > 0) && (blocksize <= 2048) && (0 == align)){
        datablksize = sd_datablocksize_get(blocksize);
        /* 发送CMD16(SET_BLOCKLEN)设置块长度 */
        sdio_command_response_config(SD_CMD_SET_BLOCKLEN, (uint32_t)blocksize, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        
        /* 检查错误 */
        status = r1_error_check(SD_CMD_SET_BLOCKLEN);
        if(SD_OK != status){
            return status;
        }
    }else{
        status = SD_PARAMETER_INVALID;
        return status;
    }
    
    stopcondition = 0;
    totalnumber_bytes = blocksize;
    
    /* 配置SDIO数据传输 */
    sdio_data_config(SD_DATATIMEOUT, totalnumber_bytes, datablksize);
    sdio_data_transfer_config(SDIO_TRANSDIRECTION_TOSDIO, SDIO_TRANSMODE_BLOCK);
    sdio_dsm_enable();
    
    /* 发送CMD17(READ_SINGLE_BLOCK)读取一个块 */
    sdio_command_response_config(SD_CMD_READ_SINGLE_BLOCK, (uint32_t)readaddr, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    /* 检查错误 */
    status = r1_error_check(SD_CMD_READ_SINGLE_BLOCK);
    if(SD_OK != status){
        return status;
    }
    
    if(SD_POLLING_MODE == transmode){
        /* 轮询模式 */
        while(!sdio_flag_get(SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTBLKEND | SDIO_FLAG_STBITE)){
            if(RESET != sdio_flag_get(SDIO_FLAG_RFH)){
                /* 至少8个字可读 */
                for(count = 0; count < SD_FIFOHALF_WORDS; count++){
                    *(ptempbuff + count) = sdio_data_read();
                }
                ptempbuff += SD_FIFOHALF_WORDS;
            }
        }
        
        /* 检查错误并返回 */
        if(RESET != sdio_flag_get(SDIO_FLAG_DTCRCERR)){
            status = SD_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO_FLAG_DTCRCERR);
            return status;
        }else if(RESET != sdio_flag_get(SDIO_FLAG_DTTMOUT)){
            status = SD_DATA_TIMEOUT;
            sdio_flag_clear(SDIO_FLAG_DTTMOUT);
            return status;
        }else if(RESET != sdio_flag_get(SDIO_FLAG_RXORE)){
            status = SD_RX_OVERRUN_ERROR;
            sdio_flag_clear(SDIO_FLAG_RXORE);
            return status;
        }else if(RESET != sdio_flag_get(SDIO_FLAG_STBITE)){
            status = SD_START_BIT_ERROR;
            sdio_flag_clear(SDIO_FLAG_STBITE);
            return status;
        }
        while(RESET != sdio_flag_get(SDIO_FLAG_RXDTVAL)){
            *ptempbuff = sdio_data_read();
            ++ptempbuff;
        }
        /* 清除SDIO_INTC标志 */
        sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    }else if(SD_DMA_MODE == transmode){
        /* DMA模式 */
        /* 使能SDIO相应中断和DMA功能 */
        sdio_interrupt_enable(SDIO_INT_CCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_RXORE | SDIO_INT_DTEND | SDIO_INT_STBITE);
        sdio_dma_enable();
        dma_receive_config(preadbuffer, blocksize);
        timeout = 100000;
        while((RESET == dma_flag_get(DMA1, DMA_CH3, DMA_FLAG_FTF)) && (timeout > 0)){
            timeout--;
            if(0 == timeout){
                return SD_ERROR;
            }
        }
    }else{
        status = SD_PARAMETER_INVALID;
    }
    return status;
}

/*!
    \brief      从卡的指定地址读取多个数据块到缓冲区
    \param[out] preadbuffer: 存放读取数据的指针
    \param[in]  readaddr: 读取地址
    \param[in]  blocksize: 数据块大小
    \param[in]  blocksnumber: 要读取的块数量
    \retval     sd_error_enum
*/
sd_error_enum sd_multiblocks_read(uint32_t *preadbuffer, uint32_t readaddr, uint16_t blocksize, uint32_t blocksnumber)
{
    /* 初始化变量 */
    sd_error_enum status = SD_OK;
    uint32_t count = 0, align = 0, datablksize = SDIO_DATABLOCKSIZE_1BYTE, *ptempbuff = preadbuffer;
    __IO uint32_t timeout = 0;
    
    if(NULL == preadbuffer){
        status = SD_PARAMETER_INVALID;
        return status;
    }
    
    transerror = SD_OK;
    transend = 0;
    totalnumber_bytes = 0;
    /* 清除所有DSM配置 */
    sdio_data_config(0, 0, SDIO_DATABLOCKSIZE_1BYTE);
    sdio_data_transfer_config(SDIO_TRANSDIRECTION_TOCARD, SDIO_TRANSMODE_BLOCK);
    sdio_dsm_disable();
    sdio_dma_disable();
    
    /* 检查卡是否锁定 */
    if(sdio_response_get(SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED){
        status = SD_LOCK_UNLOCK_FAILED;
        return status;
    }
    
    /* SDHC卡块大小固定512字节 */
    if (SDIO_HIGH_CAPACITY_SD_CARD == cardtype)
    {
        blocksize = 512;
        readaddr /= 512;
    }
    
    align = blocksize & (blocksize - 1);
    if((blocksize > 0) && (blocksize <= 2048) && (0 == align)){
        datablksize = sd_datablocksize_get(blocksize);
        /* 发送CMD16(SET_BLOCKLEN) */
        sdio_command_response_config(SD_CMD_SET_BLOCKLEN, (uint32_t)blocksize, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        
        status = r1_error_check(SD_CMD_SET_BLOCKLEN);
        if(SD_OK != status){
            return status;
        }
    }else{
        status = SD_PARAMETER_INVALID;
        return status;
    }
    
    if(blocksnumber > 1){
        if(blocksnumber * blocksize > SD_MAX_DATA_LENGTH){
            /* 超出最大长度 */
            status = SD_PARAMETER_INVALID;
            return status;
        }
        
        stopcondition = 1;
        totalnumber_bytes = blocksnumber * blocksize;
        
        /* 配置SDIO数据传输 */
        sdio_data_config(SD_DATATIMEOUT, totalnumber_bytes, datablksize);
        sdio_data_transfer_config(SDIO_TRANSDIRECTION_TOSDIO, SDIO_TRANSMODE_BLOCK);
        sdio_dsm_enable();
        
        /* 发送CMD18(READ_MULTIPLE_BLOCK) */
        sdio_command_response_config(SD_CMD_READ_MULTIPLE_BLOCK, readaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        status = r1_error_check(SD_CMD_READ_MULTIPLE_BLOCK);
        if(SD_OK != status){
            return status;
        }
        
        if(SD_POLLING_MODE == transmode){
            /* 轮询模式 */
            while(!sdio_flag_get(SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTEND | SDIO_FLAG_STBITE)){
                if(RESET != sdio_flag_get(SDIO_FLAG_RFH)){
                    for(count = 0; count < SD_FIFOHALF_WORDS; count++){
                        *(ptempbuff + count) = sdio_data_read();
                    }
                    ptempbuff += SD_FIFOHALF_WORDS;
                }
            }
            
            /* 检查错误 */
            if(RESET != sdio_flag_get(SDIO_FLAG_DTCRCERR)){
                status = SD_DATA_CRC_ERROR;
                sdio_flag_clear(SDIO_FLAG_DTCRCERR);
                return status;
            }else if(RESET != sdio_flag_get(SDIO_FLAG_DTTMOUT)){
                status = SD_DATA_TIMEOUT;
                sdio_flag_clear(SDIO_FLAG_DTTMOUT);
                return status;
            }else if(RESET != sdio_flag_get(SDIO_FLAG_RXORE)){
                status = SD_RX_OVERRUN_ERROR;
                sdio_flag_clear(SDIO_FLAG_RXORE);
                return status;
            }else if(RESET != sdio_flag_get(SDIO_FLAG_STBITE)){
                status = SD_START_BIT_ERROR;
                sdio_flag_clear(SDIO_FLAG_STBITE);
                return status;
            }
            while(RESET != sdio_flag_get(SDIO_FLAG_RXDTVAL)){
                *ptempbuff = sdio_data_read();
                ++ptempbuff;
            }
            
            if(RESET != sdio_flag_get(SDIO_FLAG_DTEND)){
                if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == cardtype) || 
                    (SDIO_HIGH_CAPACITY_SD_CARD == cardtype)){
                    /* 发送CMD12停止传输 */
                    sdio_command_response_config(SD_CMD_STOP_TRANSMISSION, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
                    sdio_wait_type_set(SDIO_WAITTYPE_NO);
                    sdio_csm_enable();
                    status = r1_error_check(SD_CMD_STOP_TRANSMISSION);
                    if(SD_OK != status){
                        return status;
                    }
                }
            }
            sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
        }else if(SD_DMA_MODE == transmode){
            /* DMA模式 */
            sdio_interrupt_enable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_RXORE | SDIO_INT_DTEND | SDIO_INT_STBITE);
            sdio_dma_enable();
            dma_receive_config(preadbuffer, totalnumber_bytes);
            
            timeout = 100000;
            while((RESET == dma_flag_get(DMA1, DMA_CH3, DMA_FLAG_FTF)) && (timeout > 0)){
                timeout--;
                if(0 == timeout){
                    return SD_ERROR;
                }
            }
            while((0 == transend) && (SD_OK == transerror)){
            }
            if(SD_OK != transerror){
                return transerror;
            }
        }else{
            status = SD_PARAMETER_INVALID;
        }
    }
    return status;
}

/*!
    \brief      将一个数据块写入卡的指定地址
    \param[in]  pwritebuffer: 存放写入数据的指针
    \param[in]  writeaddr: 写入地址
    \param[in]  blocksize: 数据块大小
    \param[out] 无
    \retval     sd_error_enum
*/
sd_error_enum sd_block_write(uint32_t *pwritebuffer, uint32_t writeaddr, uint16_t blocksize)
{
    /* 初始化变量 */
    sd_error_enum status = SD_OK;
    uint8_t cardstate = 0;
    uint32_t count = 0, align = 0, datablksize = SDIO_DATABLOCKSIZE_1BYTE, *ptempbuff = pwritebuffer;
    uint32_t transbytes = 0, restwords = 0, response = 0;
    __IO uint32_t timeout = 0;
    
    if(NULL == pwritebuffer){
        status = SD_PARAMETER_INVALID;
        return status;
    }
    
    transerror = SD_OK;
    transend = 0;
    totalnumber_bytes = 0;
    /* 清除所有DSM配置 */
    sdio_data_config(0, 0, SDIO_DATABLOCKSIZE_1BYTE);
    sdio_data_transfer_config(SDIO_TRANSDIRECTION_TOCARD, SDIO_TRANSMODE_BLOCK);
    sdio_dsm_disable();
    sdio_dma_disable();
    
    /* 检查卡是否锁定 */
    if(sdio_response_get(SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED){
        status = SD_LOCK_UNLOCK_FAILED;
        return status;
    }
    
    /* SDHC卡块大小固定512字节 */
    if (SDIO_HIGH_CAPACITY_SD_CARD == cardtype)
    {
        blocksize = 512;
        writeaddr /= 512;
    }
    
    align = blocksize & (blocksize - 1);
    if((blocksize > 0) && (blocksize <= 2048) && (0 == align)){
        datablksize = sd_datablocksize_get(blocksize);
        /* 发送CMD16 */
        sdio_command_response_config(SD_CMD_SET_BLOCKLEN, (uint32_t)blocksize, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        
        status = r1_error_check(SD_CMD_SET_BLOCKLEN);
        if(SD_OK != status){
            return status;
        }
    }else{
        status = SD_PARAMETER_INVALID;
        return status;
    }
    
    /* 发送CMD13(SEND_STATUS)查询卡状态 */
    sdio_command_response_config(SD_CMD_SEND_STATUS, (uint32_t)sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_SEND_STATUS);
    if(SD_OK != status){
        return status;
    }
    
    response = sdio_response_get(SDIO_RESPONSE0);
    timeout = 100000;
    
    while((0 == (response & SD_R1_READY_FOR_DATA)) && (timeout > 0)){
        /* 继续轮询卡状态直到缓冲区空或超时 */
        --timeout;
        sdio_command_response_config(SD_CMD_SEND_STATUS, (uint32_t)sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        status = r1_error_check(SD_CMD_SEND_STATUS);
        if(SD_OK != status){
            return status;
        }
        response = sdio_response_get(SDIO_RESPONSE0);
    }
    if(0 == timeout){
        return SD_ERROR;
    }
    
    /* 发送CMD24(WRITE_BLOCK)写入一个块 */
    sdio_command_response_config(SD_CMD_WRITE_BLOCK, writeaddr, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_WRITE_BLOCK);
    if(SD_OK != status){
        return status;
    }
    
    stopcondition = 0;
    totalnumber_bytes = blocksize;
    
    /* 配置SDIO数据传输 */
    sdio_data_config(SD_DATATIMEOUT, totalnumber_bytes, datablksize);
    sdio_data_transfer_config(SDIO_TRANSDIRECTION_TOCARD, SDIO_TRANSMODE_BLOCK);
    sdio_dsm_enable();
    
    if(SD_POLLING_MODE == transmode){
        /* 轮询模式 */
        while(!sdio_flag_get(SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_TXURE | SDIO_FLAG_DTBLKEND | SDIO_FLAG_STBITE)){
            if(RESET != sdio_flag_get(SDIO_FLAG_TFH)){
                /* 至少8个字可写 */
                if((totalnumber_bytes - transbytes) < SD_FIFOHALF_BYTES){
                    restwords = (totalnumber_bytes - transbytes)/4 + (((totalnumber_bytes - transbytes)%4 == 0) ? 0 : 1);
                    for(count = 0; count < restwords; count++){
                        sdio_data_write(*ptempbuff);
                        ++ptempbuff;
                        transbytes += 4;
                    }
                }else{
                    for(count = 0; count < SD_FIFOHALF_WORDS; count++){
                        sdio_data_write(*(ptempbuff + count));
                    }
                    ptempbuff += SD_FIFOHALF_WORDS;
                    transbytes += SD_FIFOHALF_BYTES;
                }
            }
        }
        
        /* 检查错误 */
        if(RESET != sdio_flag_get(SDIO_FLAG_DTCRCERR)){
            status = SD_DATA_CRC_ERROR;
            sdio_flag_clear(SDIO_FLAG_DTCRCERR);
            return status;
        }else if(RESET != sdio_flag_get(SDIO_FLAG_DTTMOUT)){
            status = SD_DATA_TIMEOUT;
            sdio_flag_clear(SDIO_FLAG_DTTMOUT);
            return status;
        }else if(RESET != sdio_flag_get(SDIO_FLAG_TXURE)){
            status = SD_TX_UNDERRUN_ERROR;
            sdio_flag_clear(SDIO_FLAG_TXURE);
            return status;
        }else if(RESET != sdio_flag_get(SDIO_FLAG_STBITE)){
            status = SD_START_BIT_ERROR;
            sdio_flag_clear(SDIO_FLAG_STBITE);
            return status;
        }
    }else if(SD_DMA_MODE == transmode){
        /* DMA模式 */
        sdio_interrupt_enable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_TXURE | SDIO_INT_DTEND | SDIO_INT_STBITE);
        dma_transfer_config(pwritebuffer, blocksize);
        sdio_dma_enable();
        timeout = 100000;
        while((RESET == dma_flag_get(DMA1, DMA_CH3, DMA_FLAG_FTF)) && (timeout > 0)){
            timeout--;
            if(0 == timeout){
                return SD_ERROR;
            }
        }
        while ((0 == transend) && (SD_OK == transerror)){
        }

        if (SD_OK != transerror){
            return transerror;
        }
    }else{
        status = SD_PARAMETER_INVALID;
        return status;
    }
    
    /* 清除SDIO_INTC标志 */
    sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    /* 获取卡状态并等待离开编程/接收状态 */
    status = sd_card_state_get(&cardstate);
    while((SD_OK == status) && ((SD_CARDSTATE_PROGRAMMING == cardstate) || (SD_CARDSTATE_RECEIVING == cardstate))){
        status = sd_card_state_get(&cardstate);
    }
    return status;
}

/*!
    \brief      将多个数据块写入卡的指定地址
    \param[in]  pwritebuffer: 存放写入数据的指针
    \param[in]  writeaddr: 写入地址
    \param[in]  blocksize: 数据块大小
    \param[in]  blocksnumber: 要写入的块数量
    \param[out] 无
    \retval     sd_error_enum
*/
sd_error_enum sd_multiblocks_write(uint32_t *pwritebuffer, uint32_t writeaddr, uint16_t blocksize, uint32_t blocksnumber)
{
    /* 初始化变量 */
    sd_error_enum status = SD_OK;
    uint8_t cardstate = 0;
    uint32_t count = 0, align = 0, datablksize = SDIO_DATABLOCKSIZE_1BYTE, *ptempbuff = pwritebuffer;
    uint32_t transbytes = 0, restwords = 0;
    __IO uint32_t timeout = 0;
    
    if(NULL == pwritebuffer){
        status = SD_PARAMETER_INVALID;
        return status;
    }
    
    transerror = SD_OK;
    transend = 0;
    totalnumber_bytes = 0;
    /* 清除所有DSM配置 */
    sdio_data_config(0, 0, SDIO_DATABLOCKSIZE_1BYTE);
    sdio_data_transfer_config(SDIO_TRANSDIRECTION_TOCARD, SDIO_TRANSMODE_BLOCK);
    sdio_dsm_disable();
    sdio_dma_disable();
    
    /* 检查卡是否锁定 */
    if(sdio_response_get(SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED){
        status = SD_LOCK_UNLOCK_FAILED;
        return status;
    }
    
    /* SDHC卡块大小固定512字节 */
    if (SDIO_HIGH_CAPACITY_SD_CARD == cardtype)
    {
        blocksize = 512;
        writeaddr /= 512;
    }
    
    align = blocksize & (blocksize - 1);
    if((blocksize > 0) && (blocksize <= 2048) && (0 == align)){
        datablksize = sd_datablocksize_get(blocksize);
        /* 发送CMD16 */
        sdio_command_response_config(SD_CMD_SET_BLOCKLEN, (uint32_t)blocksize, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        
        status = r1_error_check(SD_CMD_SET_BLOCKLEN);
        if(SD_OK != status){
            return status;
        }
    }else{
        status = SD_PARAMETER_INVALID;
        return status;
    }
    
    /* 发送CMD13 */
    sdio_command_response_config(SD_CMD_SEND_STATUS, (uint32_t)sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_SEND_STATUS);
    if(SD_OK != status){
        return status;
    }
    
    if(blocksnumber > 1){
        if(blocksnumber * blocksize > SD_MAX_DATA_LENGTH){
            status = SD_PARAMETER_INVALID;
            return status;
        }
        
        if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == cardtype) || 
            (SDIO_HIGH_CAPACITY_SD_CARD == cardtype)){
            /* 发送CMD55 */
            sdio_command_response_config(SD_CMD_APP_CMD, (uint32_t)sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO_WAITTYPE_NO);
            sdio_csm_enable();
            status = r1_error_check(SD_CMD_APP_CMD);
            if(SD_OK != status){
                return status;
            }
            
            /* 发送ACMD23设置预擦除块数 */
            sdio_command_response_config(SD_APPCMD_SET_WR_BLK_ERASE_COUNT, blocksnumber, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO_WAITTYPE_NO);
            sdio_csm_enable();
            status = r1_error_check(SD_APPCMD_SET_WR_BLK_ERASE_COUNT);
            if(SD_OK != status){
                return status;
            }
        }
        /* 发送CMD25(WRITE_MULTIPLE_BLOCK)连续写入多块 */
        sdio_command_response_config(SD_CMD_WRITE_MULTIPLE_BLOCK, writeaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        status = r1_error_check(SD_CMD_WRITE_MULTIPLE_BLOCK);
        if(SD_OK != status){
            return status;
        }
        
        stopcondition = 1;
        totalnumber_bytes = blocksnumber * blocksize;
        
        /* 配置SDIO数据传输 */
        sdio_data_config(SD_DATATIMEOUT, totalnumber_bytes, datablksize);
        sdio_data_transfer_config(SDIO_TRANSDIRECTION_TOCARD, SDIO_TRANSMODE_BLOCK);
        sdio_dsm_enable();
        
        if(SD_POLLING_MODE == transmode){
            /* 轮询模式 */
            while(!sdio_flag_get(SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_TXURE | SDIO_FLAG_DTEND | SDIO_FLAG_STBITE)){
                if(RESET != sdio_flag_get(SDIO_FLAG_TFH)){
                    if(!((totalnumber_bytes - transbytes) < SD_FIFOHALF_BYTES)){
                        for(count = 0; count < SD_FIFOHALF_WORDS; count++){
                            sdio_data_write(*(ptempbuff + count));
                        }
                        ptempbuff += SD_FIFOHALF_WORDS;
                        transbytes += SD_FIFOHALF_BYTES;
                    }else{
                        restwords = (totalnumber_bytes - transbytes)/4 + (((totalnumber_bytes - transbytes)%4 == 0) ? 0 : 1);
                        for(count = 0; count < restwords; count++){
                            sdio_data_write(*ptempbuff);
                            ++ptempbuff;
                            transbytes += 4;
                        }
                    }
                }
            }
            
            /* 检查错误 */
            if(RESET != sdio_flag_get(SDIO_FLAG_DTCRCERR)){
                status = SD_DATA_CRC_ERROR;
                sdio_flag_clear(SDIO_FLAG_DTCRCERR);
                return status;
            }else if(RESET != sdio_flag_get(SDIO_FLAG_DTTMOUT)){
                status = SD_DATA_TIMEOUT;
                sdio_flag_clear(SDIO_FLAG_DTTMOUT);
                return status;
            }else if(RESET != sdio_flag_get(SDIO_FLAG_TXURE)){
                status = SD_TX_UNDERRUN_ERROR;
                sdio_flag_clear(SDIO_FLAG_TXURE);
                return status;
            }else if(RESET != sdio_flag_get(SDIO_FLAG_STBITE)){
                status = SD_START_BIT_ERROR;
                sdio_flag_clear(SDIO_FLAG_STBITE);
                return status;
            }
            
            if(RESET != sdio_flag_get(SDIO_FLAG_DTEND)){
                if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == cardtype) || 
                    (SDIO_HIGH_CAPACITY_SD_CARD == cardtype)){
                    /* 发送CMD12停止传输 */
                    sdio_command_response_config(SD_CMD_STOP_TRANSMISSION, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
                    sdio_wait_type_set(SDIO_WAITTYPE_NO);
                    sdio_csm_enable();
                    status = r1_error_check(SD_CMD_STOP_TRANSMISSION);
                    if(SD_OK != status){
                        return status;
                    }
                }
            }
            sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
        }else if(SD_DMA_MODE == transmode){
            /* DMA模式 */
            sdio_interrupt_enable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_TXURE | SDIO_INT_DTEND | SDIO_INT_STBITE);
            sdio_dma_enable();
            dma_transfer_config(pwritebuffer, totalnumber_bytes);
            
            timeout = 200000;
            while((RESET == dma_flag_get(DMA1, DMA_CH3, DMA_FLAG_FTF) && (timeout > 0))){
                timeout--;
                if(0 == timeout){
                    return SD_ERROR;
                }
            }
            while((0 == transend) && (SD_OK == transerror)){
            }
            if(SD_OK != transerror){
                return transerror;
            }
        }else{
            status = SD_PARAMETER_INVALID;
            return status;
        }
    }
    
    /* 清除SDIO_INTC标志 */
    sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    /* 获取卡状态并等待离开编程/接收状态 */
    status = sd_card_state_get(&cardstate);
    while((SD_OK == status) && ((SD_CARDSTATE_PROGRAMMING == cardstate) || (SD_CARDSTATE_RECEIVING == cardstate))){
        status = sd_card_state_get(&cardstate);
    }
    return status;
}

/*!
    \brief      擦除卡上的连续区域
    \param[in]  startaddr: 起始地址
    \param[in]  endaddr: 结束地址
    \param[out] 无
    \retval     sd_error_enum
*/
sd_error_enum sd_erase(uint32_t startaddr, uint32_t endaddr)
{
    /* 初始化变量 */
    sd_error_enum status = SD_OK;
    uint32_t count = 0, clkdiv = 0;
    __IO uint32_t delay = 0;
    uint8_t cardstate = 0, tempbyte = 0;
    uint16_t tempccc = 0;
    
    /* 从CSD获取命令类支持 */
    tempbyte = (uint8_t)((sd_csd[1] & SD_MASK_24_31BITS) >> 24);
    tempccc = (uint16_t)((uint16_t)tempbyte << 4);
    tempbyte = (uint8_t)((sd_csd[1] & SD_MASK_16_23BITS) >> 16);
    tempccc |= (uint16_t)((uint16_t)(tempbyte & 0xF0) >> 4);
    if(0 == (tempccc & SD_CCC_ERASE)){
        /* 不支持擦除命令 */
        status = SD_FUNCTION_UNSUPPORTED;
        return status;
    }
    clkdiv = (SDIO_CLKCTL & SDIO_CLKCTL_DIV);
    clkdiv += ((SDIO_CLKCTL & SDIO_CLKCTL_DIV8)>>31)*256;
    clkdiv += 2;
    delay = 168000 / clkdiv;
    
    /* 检查卡是否锁定 */
    if (sdio_response_get(SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED)
    {
        status = SD_LOCK_UNLOCK_FAILED;
        return(status);
    }
    
    /* SDHC卡块大小固定512字节 */
    if (SDIO_HIGH_CAPACITY_SD_CARD == cardtype)
    {
        startaddr /= 512;
        endaddr /= 512;
    }
    
    if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == cardtype) || 
        (SDIO_HIGH_CAPACITY_SD_CARD == cardtype)){
        /* 发送CMD32设置擦除起始块 */
        sdio_command_response_config(SD_CMD_ERASE_WR_BLK_START, startaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        status = r1_error_check(SD_CMD_ERASE_WR_BLK_START);
        if(SD_OK != status){
            return status;
        }
        
        /* 发送CMD33设置擦除结束块 */
        sdio_command_response_config(SD_CMD_ERASE_WR_BLK_END, endaddr, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        status = r1_error_check(SD_CMD_ERASE_WR_BLK_END);
        if(SD_OK != status){
            return status;
        }
    }
    
    /* 发送CMD38执行擦除 */
    sdio_command_response_config(SD_CMD_ERASE, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_ERASE);
    if(SD_OK != status){
        return status;
    }
    /* 等待擦除完成（延时） */
    for(count = 0; count < delay; count++){
    }
    /* 获取卡状态并等待离开编程/接收状态 */
    status = sd_card_state_get(&cardstate);
    while((SD_OK == status) && ((SD_CARDSTATE_PROGRAMMING == cardstate) || (SD_CARDSTATE_RECEIVING == cardstate))){
        status = sd_card_state_get(&cardstate);
    }
    return status;
}

/*!
    \brief      处理所有SDIO中断标志
    \param[in]  无
    \param[out] 无
    \retval     sd_error_enum
*/
sd_error_enum sd_interrupts_process(void)
{
    transerror = SD_OK;
    if(RESET != sdio_interrupt_flag_get(SDIO_INT_DTEND)){
        /* 多块操作时发送CMD12停止数据传输 */
        if(1 == stopcondition){
            transerror = sd_transfer_stop();
        }else{
            transerror = SD_OK;
        }
        sdio_interrupt_flag_clear(SDIO_INT_DTEND);
        /* 禁用所有中断 */
        sdio_interrupt_disable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND | SDIO_INT_STBITE | 
                                SDIO_INT_TFH | SDIO_INT_RFH | SDIO_INT_TXURE | SDIO_INT_RXORE);
        transend = 1;
        number_bytes = 0;
        return transerror;
    }
    
    if(RESET != sdio_interrupt_flag_get(SDIO_INT_DTCRCERR)){
        sdio_interrupt_flag_clear(SDIO_INT_DTCRCERR);
        sdio_interrupt_disable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND | SDIO_INT_STBITE | 
                                SDIO_INT_TFH | SDIO_INT_RFH | SDIO_INT_TXURE | SDIO_INT_RXORE);
        number_bytes = 0;
        transerror = SD_DATA_CRC_ERROR;
        return transerror;
    }
    
    if(RESET != sdio_interrupt_flag_get(SDIO_INT_DTTMOUT)){
        sdio_interrupt_flag_clear(SDIO_INT_DTTMOUT);
        sdio_interrupt_disable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND | SDIO_INT_STBITE | 
                                SDIO_INT_TFH | SDIO_INT_RFH | SDIO_INT_TXURE | SDIO_INT_RXORE);
        number_bytes = 0;
        transerror = SD_DATA_TIMEOUT;
        return transerror;
    }
    
    if(RESET != sdio_interrupt_flag_get(SDIO_INT_STBITE)){
        sdio_interrupt_flag_clear(SDIO_INT_STBITE);
        sdio_interrupt_disable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND | SDIO_INT_STBITE | 
                                SDIO_INT_TFH | SDIO_INT_RFH | SDIO_INT_TXURE | SDIO_INT_RXORE);
        number_bytes = 0;
        transerror = SD_START_BIT_ERROR;
        return transerror;
    }
    
    if(RESET != sdio_interrupt_flag_get(SDIO_INT_TXURE)){
        sdio_interrupt_flag_clear(SDIO_INT_TXURE);
        sdio_interrupt_disable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND | SDIO_INT_STBITE | 
                                SDIO_INT_TFH | SDIO_INT_RFH | SDIO_INT_TXURE | SDIO_INT_RXORE);
        number_bytes = 0;
        transerror = SD_TX_UNDERRUN_ERROR;
        return transerror;
    }
    
    if(RESET != sdio_interrupt_flag_get(SDIO_INT_RXORE)){
        sdio_interrupt_flag_clear(SDIO_INT_RXORE);
        sdio_interrupt_disable(SDIO_INT_DTCRCERR | SDIO_INT_DTTMOUT | SDIO_INT_DTEND | SDIO_INT_STBITE | 
                                SDIO_INT_TFH | SDIO_INT_RFH | SDIO_INT_TXURE | SDIO_INT_RXORE);
        number_bytes = 0;
        transerror = SD_RX_OVERRUN_ERROR;
        return transerror;
    }
    return transerror;
}

/*!
    \brief      选择或取消选择卡
    \param[in]  cardrca: 卡的RCA
    \param[out] 无
    \retval     sd_error_enum
*/
sd_error_enum sd_card_select_deselect(uint16_t cardrca)
{
    sd_error_enum status = SD_OK;
    /* 发送CMD7(SELECT/DESELECT_CARD) */
    sdio_command_response_config(SD_CMD_SELECT_DESELECT_CARD, (uint32_t)(cardrca << SD_RCA_SHIFT), SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    
    status = r1_error_check(SD_CMD_SELECT_DESELECT_CARD);
    return status;
}

/*!
    \brief      获取卡状态（R1格式32位状态字）
    \param[in]  无
    \param[out] pcardstatus: 存储卡状态的指针
    \retval     sd_error_enum
*/
sd_error_enum sd_cardstatus_get(uint32_t *pcardstatus)
{
    sd_error_enum status = SD_OK;
    if(NULL == pcardstatus){
        status = SD_PARAMETER_INVALID;
        return status;
    }
    
    /* 发送CMD13 */
    sdio_command_response_config(SD_CMD_SEND_STATUS, (uint32_t)sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_SEND_STATUS);
    if(SD_OK != status){
        return status;
    }
    
    *pcardstatus = sdio_response_get(SDIO_RESPONSE0);
    return status;
}

/*!
    \brief      获取SD状态（512位数据块）
    \param[in]  无
    \param[out] psdstatus: 存储SD状态的指针
    \retval     sd_error_enum
*/
sd_error_enum sd_sdstatus_get(uint32_t *psdstatus)
{
    sd_error_enum status = SD_OK;
    uint32_t count = 0;
    
    /* 检查卡是否锁定 */
    if (sdio_response_get(SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED){
        status = SD_LOCK_UNLOCK_FAILED;
        return(status);
    }
    
    /* 设置块长度为64字节 */
    sdio_command_response_config(SD_CMD_SET_BLOCKLEN, (uint32_t)64, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_SET_BLOCKLEN);
    if(SD_OK != status){
        return status;
    }
    
    /* 发送CMD55 */
    sdio_command_response_config(SD_CMD_APP_CMD, (uint32_t)sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_APP_CMD);
    if(SD_OK != status){
        return status;
    }
    
    /* 配置SDIO数据：64字节，从卡到主机 */
    sdio_data_config(SD_DATATIMEOUT, (uint32_t)64, SDIO_DATABLOCKSIZE_64BYTES);
    sdio_data_transfer_config(SDIO_TRANSDIRECTION_TOSDIO, SDIO_TRANSMODE_BLOCK);
    sdio_dsm_enable();
    
    /* 发送ACMD13(SD_STATUS) */
    sdio_command_response_config(SD_APPCMD_SD_STATUS, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_APPCMD_SD_STATUS);
    if(SD_OK != status){
        return status;
    }
    
    while(!sdio_flag_get(SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTBLKEND | SDIO_FLAG_STBITE)){
        if(RESET != sdio_flag_get(SDIO_FLAG_RFH)){
            for(count = 0; count < SD_FIFOHALF_WORDS; count++){
                *(psdstatus + count) = sdio_data_read();
            }
            psdstatus += SD_FIFOHALF_WORDS;
        }
    }
    
    /* 检查错误 */
    if(RESET != sdio_flag_get(SDIO_FLAG_DTCRCERR)){
        status = SD_DATA_CRC_ERROR;
        sdio_flag_clear(SDIO_FLAG_DTCRCERR);
        return status;
    }else if(RESET != sdio_flag_get(SDIO_FLAG_DTTMOUT)){
        status = SD_DATA_TIMEOUT;
        sdio_flag_clear(SDIO_FLAG_DTTMOUT);
        return status;
    }else if(RESET != sdio_flag_get(SDIO_FLAG_RXORE)){
        status = SD_RX_OVERRUN_ERROR;
        sdio_flag_clear(SDIO_FLAG_RXORE);
        return status;
    }else if(RESET != sdio_flag_get(SDIO_FLAG_STBITE)){
        status = SD_START_BIT_ERROR;
        sdio_flag_clear(SDIO_FLAG_STBITE);
        return status;
    }
    while(RESET != sdio_flag_get(SDIO_FLAG_RXDTVAL)){
        *psdstatus = sdio_data_read();
        ++psdstatus;
    }
    
    /* 清除SDIO_INTC标志 */
    sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    psdstatus -= 16;
    for(count = 0; count < 16; count++){
        psdstatus[count] = ((psdstatus[count] & SD_MASK_0_7BITS) << 24) |((psdstatus[count] & SD_MASK_8_15BITS) << 8) | 
                           ((psdstatus[count] & SD_MASK_16_23BITS) >> 8) |((psdstatus[count] & SD_MASK_24_31BITS) >> 24);
    }
    return status;
}

/*!
    \brief      停止正在进行的数据传输
    \param[in]  无
    \param[out] 无
    \retval     sd_error_enum
*/
sd_error_enum sd_transfer_stop(void)
{
    sd_error_enum status = SD_OK;
    /* 发送CMD12停止传输 */
    sdio_command_response_config(SD_CMD_STOP_TRANSMISSION, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_STOP_TRANSMISSION);
    return status;
}

/*!
    \brief      锁定或解锁SD卡
    \param[in]  lockstate: 锁定状态
      \arg        SD_LOCK: 锁定
      \arg        SD_UNLOCK: 解锁
    \param[out] 无
    \retval     sd_error_enum
*/
sd_error_enum sd_lock_unlock(uint8_t lockstate)
{
    sd_error_enum status = SD_OK;
    uint8_t cardstate = 0, tempbyte = 0;
    uint32_t pwd1 = 0, pwd2 = 0, response = 0, timeout = 0;
    uint16_t tempccc = 0;
    
    /* 从CSD获取命令类支持 */
    tempbyte = (uint8_t)((sd_csd[1] & SD_MASK_24_31BITS) >> 24);
    tempccc = (uint16_t)((uint16_t)tempbyte << 4);
    tempbyte = (uint8_t)((sd_csd[1] & SD_MASK_16_23BITS) >> 16);
    tempccc |= (uint16_t)((uint16_t)(tempbyte & 0xF0) >> 4);
    
    if(0 == (tempccc & SD_CCC_LOCK_CARD)){
        status = SD_FUNCTION_UNSUPPORTED;
        return status;
    }
    /* 密码模式 */
    pwd1 = (0x01020600|lockstate);
    pwd2 = 0x03040506;
    
    /* 清除DSM配置 */
    sdio_data_config(0, 0, SDIO_DATABLOCKSIZE_1BYTE);
    sdio_data_transfer_config(SDIO_TRANSDIRECTION_TOCARD, SDIO_TRANSMODE_BLOCK);
    sdio_dsm_disable();
    sdio_dma_disable();
    
    /* 设置块长度为8字节 */
    sdio_command_response_config(SD_CMD_SET_BLOCKLEN, (uint32_t)8, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_SET_BLOCKLEN);
    if(SD_OK != status){
        return status;
    }
    
    /* 查询卡状态 */
    sdio_command_response_config(SD_CMD_SEND_STATUS, (uint32_t)sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_SEND_STATUS);
    if(SD_OK != status){
        return status;
    }
    
    response = sdio_response_get(SDIO_RESPONSE0);
    timeout = 100000;
    while((0 == (response & SD_R1_READY_FOR_DATA)) && (timeout > 0)){
        --timeout;
        sdio_command_response_config(SD_CMD_SEND_STATUS, (uint32_t)sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
        sdio_wait_type_set(SDIO_WAITTYPE_NO);
        sdio_csm_enable();
        status = r1_error_check(SD_CMD_SEND_STATUS);
        if(SD_OK != status){
            return status;
        }
        response = sdio_response_get(SDIO_RESPONSE0);
    }
    if(0 == timeout){
        return SD_ERROR;
    }
    
    /* 发送CMD42(LOCK_UNLOCK) */
    sdio_command_response_config(SD_CMD_LOCK_UNLOCK, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_LOCK_UNLOCK);
    if(SD_OK != status){
        return status;
    }
    
    response = sdio_response_get(SDIO_RESPONSE0);
    
    /* 配置SDIO数据：8字节 */
    sdio_data_config(SD_DATATIMEOUT, (uint32_t)8, SDIO_DATABLOCKSIZE_8BYTES);
    sdio_data_transfer_config(SDIO_TRANSDIRECTION_TOCARD, SDIO_TRANSMODE_BLOCK);
    sdio_dsm_enable();
    
    /* 写入密码 */
    sdio_data_write(pwd1);
    sdio_data_write(pwd2);
    
    /* 检查错误 */
    if(RESET != sdio_flag_get(SDIO_FLAG_DTCRCERR)){
        status = SD_DATA_CRC_ERROR;
        sdio_flag_clear(SDIO_FLAG_DTCRCERR);
        return status;
    }else if(RESET != sdio_flag_get(SDIO_FLAG_DTTMOUT)){
        status = SD_DATA_TIMEOUT;
        sdio_flag_clear(SDIO_FLAG_DTTMOUT);
        return status;
    }else if(RESET != sdio_flag_get(SDIO_FLAG_TXURE)){
        status = SD_TX_UNDERRUN_ERROR;
        sdio_flag_clear(SDIO_FLAG_TXURE);
        return status;
    }else if(RESET != sdio_flag_get(SDIO_FLAG_STBITE)){
        status = SD_START_BIT_ERROR;
        sdio_flag_clear(SDIO_FLAG_STBITE);
        return status;
    }
    
    /* 清除标志并等待卡退出编程/接收状态 */
    sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    status = sd_card_state_get(&cardstate);
    while((SD_OK == status) && ((SD_CARDSTATE_PROGRAMMING == cardstate) || (SD_CARDSTATE_RECEIVING == cardstate))){
        status = sd_card_state_get(&cardstate);
    }
    return status;
}

/*!
    \brief      获取数据传输状态
    \param[in]  无
    \param[out] 无
    \retval     sd_transfer_state_enum
*/
sd_transfer_state_enum sd_transfer_state_get(void)
{
    sd_transfer_state_enum transtate = SD_NO_TRANSFER;
    if(RESET != sdio_flag_get(SDIO_FLAG_TXRUN | SDIO_FLAG_RXRUN)){
        transtate = SD_TRANSFER_IN_PROGRESS;
    }
    return transtate;
}

/*!
    \brief      获取SD卡容量（KB）
    \param[in]  无
    \param[out] 无
    \retval     容量（KB）
*/
uint32_t sd_card_capacity_get(void)
{
    uint8_t tempbyte = 0, devicesize_mult = 0, readblklen = 0;
    uint32_t capacity = 0, devicesize = 0;
    if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == cardtype)){
        /* 计算c_size */
        tempbyte = (uint8_t)((sd_csd[1] & SD_MASK_8_15BITS) >> 8);
        devicesize |= (uint32_t)((uint32_t)(tempbyte & 0x03) << 10);
        tempbyte = (uint8_t)(sd_csd[1] & SD_MASK_0_7BITS);
        devicesize |= (uint32_t)((uint32_t)tempbyte << 2);
        tempbyte = (uint8_t)((sd_csd[2] & SD_MASK_24_31BITS) >> 24);
        devicesize |= (uint32_t)((uint32_t)(tempbyte & 0xC0) >> 6);
        
        /* 计算c_size_mult */
        tempbyte = (uint8_t)((sd_csd[2] & SD_MASK_16_23BITS) >> 16);
        devicesize_mult = (tempbyte & 0x03) << 1;
        tempbyte = (uint8_t)((sd_csd[2] & SD_MASK_8_15BITS) >> 8);
        devicesize_mult |= (tempbyte & 0x80) >> 7;
        
        /* 计算read_bl_len */
        tempbyte = (uint8_t)((sd_csd[1] & SD_MASK_16_23BITS) >> 16);
        readblklen = tempbyte & 0x0F;
        
        /* 容量 = BLOCKNR*BLOCK_LEN */
        capacity = (devicesize + 1)*(1 << (devicesize_mult + 2));
        capacity *= (1 << readblklen);
        capacity /= 1024;
    }else if(SDIO_HIGH_CAPACITY_SD_CARD == cardtype){
        /* 计算c_size */
        tempbyte = (uint8_t)(sd_csd[1] & SD_MASK_0_7BITS);
        devicesize = (uint32_t)((uint32_t)(tempbyte & 0x3F) << 16);
        tempbyte = (uint8_t)((sd_csd[2] & SD_MASK_24_31BITS) >> 24);
        devicesize |= (uint32_t)((uint32_t)tempbyte << 8);
        tempbyte = (uint8_t)((sd_csd[2] & SD_MASK_16_23BITS) >> 16);
        devicesize |= (uint32_t)tempbyte;
        
        /* 容量 = (c_size+1)*512KByte */
        capacity = (devicesize + 1)*512;
    }
    return capacity;
}

/*!
    \brief      基于CID和CSD获取卡的详细信息
    \param[in]  无
    \param[out] pcardinfo: 存储卡信息的指针
    \retval     sd_error_enum
*/
sd_error_enum sd_card_information_get(sd_card_info_struct *pcardinfo)
{
    sd_error_enum status = SD_OK;
    uint8_t tempbyte = 0;
    
    if(NULL == pcardinfo){
        status = SD_PARAMETER_INVALID;
        return status;
    }
    
    /* 存储卡类型和RCA */
    pcardinfo->card_type = cardtype;
    pcardinfo->card_rca = sd_rca;
    
    /* CID字节解析（制造商ID、OEM/应用ID等） */
    tempbyte = (uint8_t)((sd_cid[0] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_cid.mid = tempbyte;
    
    tempbyte = (uint8_t)((sd_cid[0] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_cid.oid = (uint16_t)((uint16_t)tempbyte << 8);
    
    tempbyte = (uint8_t)((sd_cid[0] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_cid.oid |= (uint16_t)tempbyte;
    
    tempbyte = (uint8_t)(sd_cid[0] & SD_MASK_0_7BITS);
    pcardinfo->card_cid.pnm0 = (uint32_t)((uint32_t)tempbyte << 24);
    
    tempbyte = (uint8_t)((sd_cid[1] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_cid.pnm0 |= (uint32_t)((uint32_t)tempbyte << 16);
    
    tempbyte = (uint8_t)((sd_cid[1] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_cid.pnm0 |= (uint32_t)((uint32_t)tempbyte << 8);
    
    tempbyte = (uint8_t)((sd_cid[1] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_cid.pnm0 |= (uint32_t)(tempbyte);
    
    tempbyte = (uint8_t)(sd_cid[1] & SD_MASK_0_7BITS);
    pcardinfo->card_cid.pnm1 = tempbyte;
    
    tempbyte = (uint8_t)((sd_cid[2] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_cid.prv = tempbyte;
    
    tempbyte = (uint8_t)((sd_cid[2] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_cid.psn = (uint32_t)((uint32_t)tempbyte << 24);
    
    tempbyte = (uint8_t)((sd_cid[2] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_cid.psn |= (uint32_t)((uint32_t)tempbyte << 16);
    
    tempbyte = (uint8_t)(sd_cid[2] & SD_MASK_0_7BITS);
    pcardinfo->card_cid.psn |= (uint32_t)tempbyte;
    
    tempbyte = (uint8_t)((sd_cid[3] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_cid.psn |= (uint32_t)tempbyte;
    
    tempbyte = (uint8_t)((sd_cid[3] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_cid.mdt = (uint16_t)((uint16_t)(tempbyte & 0x0F) << 8);
    
    tempbyte = (uint8_t)((sd_cid[3] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_cid.mdt |= (uint16_t)tempbyte;
    
    tempbyte = (uint8_t)(sd_cid[3] & SD_MASK_0_7BITS);
    pcardinfo->card_cid.cid_crc = (tempbyte & 0xFE) >> 1;
    
    /* CSD字节解析 */
    tempbyte = (uint8_t)((sd_csd[0] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_csd.csd_struct = (tempbyte & 0xC0) >> 6;
    
    tempbyte = (uint8_t)((sd_csd[0] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_csd.taac = tempbyte;
    
    tempbyte = (uint8_t)((sd_csd[0] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_csd.nsac = tempbyte;
    
    tempbyte = (uint8_t)(sd_csd[0] & SD_MASK_0_7BITS);
    pcardinfo->card_csd.tran_speed = tempbyte;
    
    tempbyte = (uint8_t)((sd_csd[1] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_csd.ccc = (uint16_t)((uint16_t)tempbyte << 4);
    
    tempbyte = (uint8_t)((sd_csd[1] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_csd.ccc |= (uint16_t)((uint16_t)(tempbyte & 0xF0) >> 4);
    pcardinfo->card_csd.read_bl_len = tempbyte & 0x0F;
    
    tempbyte = (uint8_t)((sd_csd[1] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_csd.read_bl_partial = (tempbyte & 0x80) >> 7;
    pcardinfo->card_csd.write_blk_misalign = (tempbyte & 0x40) >> 6;
    pcardinfo->card_csd.read_blk_misalign = (tempbyte & 0x20) >> 5;
    pcardinfo->card_csd.dsp_imp = (tempbyte & 0x10) >> 4;
    
    if((SDIO_STD_CAPACITY_SD_CARD_V1_1 == cardtype) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == cardtype)){
        /* SDSC卡，CSD版本1.0 */
        pcardinfo->card_csd.c_size = (uint32_t)((uint32_t)(tempbyte & 0x03) << 10);
        
        tempbyte = (uint8_t)(sd_csd[1] & SD_MASK_0_7BITS);
        pcardinfo->card_csd.c_size |= (uint32_t)((uint32_t)tempbyte << 2);
        
        tempbyte = (uint8_t)((sd_csd[2] & SD_MASK_24_31BITS) >> 24);
        pcardinfo->card_csd.c_size |= (uint32_t)((uint32_t)(tempbyte & 0xC0) >> 6);
        pcardinfo->card_csd.vdd_r_curr_min = (tempbyte & 0x38) >> 3;
        pcardinfo->card_csd.vdd_r_curr_max = tempbyte & 0x07;
        
        tempbyte = (uint8_t)((sd_csd[2] & SD_MASK_16_23BITS) >> 16);
        pcardinfo->card_csd.vdd_w_curr_min = (tempbyte & 0xE0) >> 5;
        pcardinfo->card_csd.vdd_w_curr_max = (tempbyte & 0x1C) >> 2;
        pcardinfo->card_csd.c_size_mult = (tempbyte & 0x03) << 1;
        
        tempbyte = (uint8_t)((sd_csd[2] & SD_MASK_8_15BITS) >> 8);
        pcardinfo->card_csd.c_size_mult |= (tempbyte & 0x80) >> 7;
        
        pcardinfo->card_blocksize = 1 << (pcardinfo->card_csd.read_bl_len);
        pcardinfo->card_capacity = pcardinfo->card_csd.c_size + 1;
        pcardinfo->card_capacity *= (1 << (pcardinfo->card_csd.c_size_mult + 2));
        pcardinfo->card_capacity *= pcardinfo->card_blocksize;
    }else if(SDIO_HIGH_CAPACITY_SD_CARD == cardtype){
        /* SDHC卡，CSD版本2.0 */
        tempbyte = (uint8_t)(sd_csd[1] & SD_MASK_0_7BITS);
        pcardinfo->card_csd.c_size = (uint32_t)((uint32_t)(tempbyte & 0x3F) << 16);
        
        tempbyte = (uint8_t)((sd_csd[2] & SD_MASK_24_31BITS) >> 24);
        pcardinfo->card_csd.c_size |= (uint32_t)((uint32_t)tempbyte << 8);
        
        tempbyte = (uint8_t)((sd_csd[2] & SD_MASK_16_23BITS) >> 16);
        pcardinfo->card_csd.c_size |= (uint32_t)tempbyte;
        
        pcardinfo->card_blocksize = 512;
        pcardinfo->card_capacity = (pcardinfo->card_csd.c_size + 1) * 512 *1024;
    }
    
    pcardinfo->card_csd.erase_blk_en = (tempbyte & 0x40) >> 6;
    pcardinfo->card_csd.sector_size = (tempbyte & 0x3F) << 1;
    
    tempbyte = (uint8_t)(sd_csd[2] & SD_MASK_0_7BITS);
    pcardinfo->card_csd.sector_size |= (tempbyte & 0x80) >> 7;
    pcardinfo->card_csd.wp_grp_size = (tempbyte & 0x7F);
    
    tempbyte = (uint8_t)((sd_csd[3] & SD_MASK_24_31BITS) >> 24);
    pcardinfo->card_csd.wp_grp_enable = (tempbyte & 0x80) >> 7;
    pcardinfo->card_csd.r2w_factor = (tempbyte & 0x1C) >> 2;
    pcardinfo->card_csd.write_bl_len = (tempbyte & 0x03) << 2;
    
    tempbyte = (uint8_t)((sd_csd[3] & SD_MASK_16_23BITS) >> 16);
    pcardinfo->card_csd.write_bl_len |= (tempbyte & 0xC0) >> 6;
    pcardinfo->card_csd.write_bl_partial = (tempbyte & 0x20) >> 5;
    
    tempbyte = (uint8_t)((sd_csd[3] & SD_MASK_8_15BITS) >> 8);
    pcardinfo->card_csd.file_format_grp = (tempbyte & 0x80) >> 7;
    pcardinfo->card_csd.copy_flag = (tempbyte & 0x40) >> 6;
    pcardinfo->card_csd.perm_write_protect = (tempbyte & 0x20) >> 5;
    pcardinfo->card_csd.tmp_write_protect = (tempbyte & 0x10) >> 4;
    pcardinfo->card_csd.file_format = (tempbyte & 0x0C) >> 2;
    
    tempbyte = (uint8_t)(sd_csd[3] & SD_MASK_0_7BITS);
    pcardinfo->card_csd.csd_crc = (tempbyte & 0xFE) >> 1;
    
    return status;
}

/*!
    \brief      检查命令发送是否出错
    \param[in]  无
    \param[out] 无
    \retval     sd_error_enum
*/
static sd_error_enum cmdsent_error_check(void)
{
    sd_error_enum status = SD_OK;
    uint32_t timeout = 100000;
    /* 等待命令发送标志 */
    while((RESET == sdio_flag_get(SDIO_FLAG_CMDSEND)) && (timeout > 0)){
        --timeout;
    }
    if(0 == timeout){
        status = SD_CMD_RESP_TIMEOUT;
        return status;
    }
    /* 清除标志 */
    sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    return status;
}

/*!
    \brief      根据R1响应内容判断错误类型
    \param[in]  resp: R1响应值
    \param[out] 无
    \retval     sd_error_enum
*/
static sd_error_enum r1_error_type_check(uint32_t resp)
{
    sd_error_enum status = SD_ERROR;
    if(resp & SD_R1_OUT_OF_RANGE){
        status = SD_OUT_OF_RANGE;
    }else if(resp & SD_R1_ADDRESS_ERROR){
        status = SD_ADDRESS_ERROR;
    }else if(resp & SD_R1_BLOCK_LEN_ERROR){
        status = SD_BLOCK_LEN_ERROR;
    }else if(resp & SD_R1_ERASE_SEQ_ERROR){
        status = SD_ERASE_SEQ_ERROR;
    }else if(resp & SD_R1_ERASE_PARAM){
        status = SD_ERASE_PARAM;
    }else if(resp & SD_R1_WP_VIOLATION){
        status = SD_WP_VIOLATION;
    }else if(resp & SD_R1_LOCK_UNLOCK_FAILED){
        status = SD_LOCK_UNLOCK_FAILED;
    }else if(resp & SD_R1_COM_CRC_ERROR){
        status = SD_COM_CRC_ERROR;
    }else if(resp & SD_R1_ILLEGAL_COMMAND){
        status = SD_ILLEGAL_COMMAND;
    }else if(resp & SD_R1_CARD_ECC_FAILED){
        status = SD_CARD_ECC_FAILED;
    }else if(resp & SD_R1_CC_ERROR){
        status = SD_CC_ERROR;
    }else if(resp & SD_R1_GENERAL_UNKNOWN_ERROR){
        status = SD_GENERAL_UNKNOWN_ERROR;
    }else if(resp & SD_R1_CSD_OVERWRITE){
        status = SD_CSD_OVERWRITE;
    }else if(resp & SD_R1_WP_ERASE_SKIP){
        status = SD_WP_ERASE_SKIP;
    }else if(resp & SD_R1_CARD_ECC_DISABLED){
        status = SD_CARD_ECC_DISABLED;
    }else if(resp & SD_R1_ERASE_RESET){
        status = SD_ERASE_RESET;
    }else if(resp & SD_R1_AKE_SEQ_ERROR){
        status = SD_AKE_SEQ_ERROR;
    }
    return status;
}

/*!
    \brief      检查R1响应是否出错
    \param[in]  cmdindex: 命令索引
    \param[out] 无
    \retval     sd_error_enum
*/
static sd_error_enum r1_error_check(uint8_t cmdindex)
{
    sd_error_enum status = SD_OK;
    uint32_t reg_status = 0, resp_r1 = 0;
    
    reg_status = SDIO_STAT;
    while(!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV))){
        reg_status = SDIO_STAT;
    }
    if(reg_status & SDIO_FLAG_CCRCERR){
        status = SD_CMD_CRC_ERROR;
        sdio_flag_clear(SDIO_FLAG_CCRCERR);
        return status;
    }else if(reg_status & SDIO_FLAG_CMDTMOUT){
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO_FLAG_CMDTMOUT);
        return status;
    }
    
    if(sdio_command_index_get() != cmdindex){
        status = SD_ILLEGAL_COMMAND;
        return status;
    }
    sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    resp_r1 = sdio_response_get(SDIO_RESPONSE0);
    if(SD_ALLZERO == (resp_r1 & SD_R1_ERROR_BITS)){
        status = SD_OK;
        return status;
    }
    
    status = r1_error_type_check(resp_r1);
    return status;
}

/*!
    \brief      检查R2响应是否出错
    \param[in]  无
    \param[out] 无
    \retval     sd_error_enum
*/
static sd_error_enum r2_error_check(void)
{
    sd_error_enum status = SD_OK;
    uint32_t reg_status = 0;
    
    reg_status = SDIO_STAT;
    while(!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV))){
        reg_status = SDIO_STAT;
    }
    if(reg_status & SDIO_FLAG_CCRCERR){
        status = SD_CMD_CRC_ERROR;
        sdio_flag_clear(SDIO_FLAG_CCRCERR);
        return status;
    }else if(reg_status & SDIO_FLAG_CMDTMOUT){
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO_FLAG_CMDTMOUT);
        return status;
    }
    sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    return status;
}

/*!
    \brief      检查R3响应是否出错
    \param[in]  无
    \param[out] 无
    \retval     sd_error_enum
*/
static sd_error_enum r3_error_check(void)
{
    sd_error_enum status = SD_OK;
    uint32_t reg_status = 0;
    
    reg_status = SDIO_STAT;
    while(!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV))){
        reg_status = SDIO_STAT;
    }
    if(reg_status & SDIO_FLAG_CMDTMOUT){
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO_FLAG_CMDTMOUT);
        return status;
    }
    sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    return status;
}

/*!
    \brief      检查R6响应是否出错并提取RCA
    \param[in]  cmdindex: 命令索引
    \param[out] prca: 存储RCA的指针
    \retval     sd_error_enum
*/
static sd_error_enum r6_error_check(uint8_t cmdindex, uint16_t *prca)
{
    sd_error_enum status = SD_OK;
    uint32_t reg_status = 0, response = 0;
    
    reg_status = SDIO_STAT;
    while(!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV))){
        reg_status = SDIO_STAT;
    }
    if(reg_status & SDIO_FLAG_CCRCERR){
        status = SD_CMD_CRC_ERROR;
        sdio_flag_clear(SDIO_FLAG_CCRCERR);
        return status;
    }else if(reg_status & SDIO_FLAG_CMDTMOUT){
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO_FLAG_CMDTMOUT);
        return status;
    }
    
    if(sdio_command_index_get() != cmdindex){
        status = SD_ILLEGAL_COMMAND;
        return status;
    }
    sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    response = sdio_response_get(SDIO_RESPONSE0);
    
    if(SD_ALLZERO == (response & (SD_R6_COM_CRC_ERROR | SD_R6_ILLEGAL_COMMAND | SD_R6_GENERAL_UNKNOWN_ERROR))){
        *prca = (uint16_t)(response >> 16);
        return status;
    }
    if(response & SD_R6_COM_CRC_ERROR){
        status = SD_COM_CRC_ERROR;
    }else if(response & SD_R6_ILLEGAL_COMMAND){
        status = SD_ILLEGAL_COMMAND;
    }else if(response & SD_R6_GENERAL_UNKNOWN_ERROR){
        status = SD_GENERAL_UNKNOWN_ERROR;
    }
    return status;
}

/*!
    \brief      检查R7响应是否出错
    \param[in]  无
    \param[out] 无
    \retval     sd_error_enum
*/
static sd_error_enum r7_error_check(void)
{
    sd_error_enum status = SD_ERROR;
    uint32_t reg_status = 0, timeout = 10000;
    
    reg_status = SDIO_STAT;
    while(!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV)) && (timeout > 0)){
        reg_status = SDIO_STAT;
        --timeout;
    }
    
    if((reg_status & SDIO_FLAG_CMDTMOUT) || (0 == timeout)){
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO_FLAG_CMDTMOUT);
        return status;
    }
    if(reg_status & SDIO_FLAG_CMDRECV){
        status = SD_OK;
        sdio_flag_clear(SDIO_FLAG_CMDRECV);
        return status;
    }
    return status;
}

/*!
    \brief      获取卡当前状态
    \param[in]  无
    \param[out] pcardstate: 存储卡状态的指针
    \retval     sd_error_enum
*/
static sd_error_enum sd_card_state_get(uint8_t *pcardstate)
{
    sd_error_enum status = SD_OK;
    __IO uint32_t reg_status = 0, response = 0;
    
    /* 发送CMD13 */
    sdio_command_response_config(SD_CMD_SEND_STATUS, (uint32_t)sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    
    reg_status = SDIO_STAT;
    while(!(reg_status & (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | SDIO_FLAG_CMDRECV))){
        reg_status = SDIO_STAT;
    }
    if(reg_status & SDIO_FLAG_CCRCERR){
        status = SD_CMD_CRC_ERROR;
        sdio_flag_clear(SDIO_FLAG_CCRCERR);
        return status;
    }else if(reg_status & SDIO_FLAG_CMDTMOUT){
        status = SD_CMD_RESP_TIMEOUT;
        sdio_flag_clear(SDIO_FLAG_CMDTMOUT);
        return status;
    }
    
    reg_status = (uint32_t)sdio_command_index_get();
    if(reg_status != (uint32_t)SD_CMD_SEND_STATUS){
        status = SD_ILLEGAL_COMMAND;
        return status;
    }
    sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    response = sdio_response_get(SDIO_RESPONSE0);
    *pcardstate = (uint8_t)((response >> 9) & 0x0000000F);
    
    if(SD_ALLZERO == (response & SD_R1_ERROR_BITS)){
        status = SD_OK;
        return status;
    }
    status = r1_error_type_check(response);
    return status;
}

/*!
    \brief      配置总线宽度（1位或4位）
    \param[in]  buswidth: 总线宽度
      \arg        SD_BUS_WIDTH_1BIT: 1位
      \arg        SD_BUS_WIDTH_4BIT: 4位
    \param[out] 无
    \retval     sd_error_enum
*/
static sd_error_enum sd_bus_width_config(uint32_t buswidth)
{
    sd_error_enum status = SD_OK;
    if(sdio_response_get(SDIO_RESPONSE0) & SD_CARDSTATE_LOCKED){
        status = SD_LOCK_UNLOCK_FAILED;
        return status;
    }
    status = sd_scr_get(sd_rca, sd_scr);
    if(SD_OK != status){
        return status;
    }
    
    if(SD_BUS_WIDTH_1BIT == buswidth){
        if(SD_ALLZERO != (sd_scr[1] & buswidth)){
            sdio_command_response_config(SD_CMD_APP_CMD, (uint32_t)sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO_WAITTYPE_NO);
            sdio_csm_enable();
            status = r1_error_check(SD_CMD_APP_CMD);
            if(SD_OK != status){
                return status;
            }
            
            sdio_command_response_config(SD_APPCMD_SET_BUS_WIDTH, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO_WAITTYPE_NO);
            sdio_csm_enable();
            status = r1_error_check(SD_APPCMD_SET_BUS_WIDTH);
        }else{
            status = SD_OPERATION_IMPROPER;
        }
        return status;
    }else if(SD_BUS_WIDTH_4BIT == buswidth){
        if(SD_ALLZERO != (sd_scr[1] & buswidth)){
            sdio_command_response_config(SD_CMD_APP_CMD, (uint32_t)sd_rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO_WAITTYPE_NO);
            sdio_csm_enable();
            status = r1_error_check(SD_CMD_APP_CMD);
            if(SD_OK != status){
                return status;
            }
            
            sdio_command_response_config(SD_APPCMD_SET_BUS_WIDTH, (uint32_t)0x2, SDIO_RESPONSETYPE_SHORT);
            sdio_wait_type_set(SDIO_WAITTYPE_NO);
            sdio_csm_enable();
            status = r1_error_check(SD_APPCMD_SET_BUS_WIDTH);
        }else{
            status = SD_OPERATION_IMPROPER;
        }
        return status;
    }else{
        status = SD_PARAMETER_INVALID;
        return status;
    }
}

/*!
    \brief      获取卡的SCR寄存器内容
    \param[in]  rca: 卡的RCA
    \param[out] pscr: 存储SCR的指针
    \retval     sd_error_enum
*/
static sd_error_enum sd_scr_get(uint16_t rca, uint32_t *pscr)
{
    sd_error_enum status = SD_OK;
    uint32_t temp_scr[2] = {0, 0}, idx_scr = 0;
    /* 设置块长度为8字节 */
    sdio_command_response_config(SD_CMD_SET_BLOCKLEN, (uint32_t)8, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_SET_BLOCKLEN);
    if(SD_OK != status){
        return status;
    }
    
    /* 发送CMD55 */
    sdio_command_response_config(SD_CMD_APP_CMD, (uint32_t)rca << SD_RCA_SHIFT, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_CMD_APP_CMD);
    if(SD_OK != status){
        return status;
    }
    
    /* 配置数据：8字节，从卡到主机 */
    sdio_data_config(SD_DATATIMEOUT, (uint32_t)8, SDIO_DATABLOCKSIZE_8BYTES);
    sdio_data_transfer_config(SDIO_TRANSMODE_BLOCK, SDIO_TRANSDIRECTION_TOSDIO);
    sdio_dsm_enable();
    
    /* 发送ACMD51 */
    sdio_command_response_config(SD_APPCMD_SEND_SCR, (uint32_t)0x0, SDIO_RESPONSETYPE_SHORT);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();
    status = r1_error_check(SD_APPCMD_SEND_SCR);
    if(SD_OK != status){
        return status;
    }
    
    while(!sdio_flag_get(SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | SDIO_FLAG_RXORE | SDIO_FLAG_DTBLKEND | SDIO_FLAG_STBITE)){
        if(RESET != sdio_flag_get(SDIO_FLAG_RXDTVAL)){
            *(temp_scr + idx_scr) = sdio_data_read();
            ++idx_scr;
        }
    }
    
    if(RESET != sdio_flag_get(SDIO_FLAG_DTCRCERR)){
        status = SD_DATA_CRC_ERROR;
        sdio_flag_clear(SDIO_FLAG_DTCRCERR);
        return status;
    }else if(RESET != sdio_flag_get(SDIO_FLAG_DTTMOUT)){
        status = SD_DATA_TIMEOUT;
        sdio_flag_clear(SDIO_FLAG_DTTMOUT);
        return status;
    }else if(RESET != sdio_flag_get(SDIO_FLAG_RXORE)){
        status = SD_RX_OVERRUN_ERROR;
        sdio_flag_clear(SDIO_FLAG_RXORE);
        return status;
    }else if(RESET != sdio_flag_get(SDIO_FLAG_STBITE)){
        status = SD_START_BIT_ERROR;
        sdio_flag_clear(SDIO_FLAG_STBITE);
        return status;
    }
    
    sdio_flag_clear(SDIO_MASK_INTC_FLAGS);
    /* 重新排列SCR值 */
    *(pscr) = ((temp_scr[1] & SD_MASK_0_7BITS) << 24) | ((temp_scr[1] & SD_MASK_8_15BITS) << 8) | 
                ((temp_scr[1] & SD_MASK_16_23BITS) >> 8) | ((temp_scr[1] & SD_MASK_24_31BITS) >> 24);
    *(pscr + 1) = ((temp_scr[0] & SD_MASK_0_7BITS) << 24) | ((temp_scr[0] & SD_MASK_8_15BITS) << 8) | 
                ((temp_scr[0] & SD_MASK_16_23BITS) >> 8) | ((temp_scr[0] & SD_MASK_24_31BITS) >> 24);
    return status;
}

/*!
    \brief      根据字节数获取SDIO数据块大小编码
    \param[in]  bytesnumber: 字节数
    \param[out] 无
    \retval     数据块大小编码
*/
static uint32_t sd_datablocksize_get(uint16_t bytesnumber)
{
    uint8_t exp_val = 0;
    while(1 != bytesnumber){
        bytesnumber >>= 1;
        ++exp_val;
    }
    return DATACTL_BLKSZ(exp_val);
}

/*!
    \brief      配置SDIO接口的GPIO引脚
    \param[in]  无
    \param[out] 无
    \retval     无
*/
static void gpio_config(void)
{
    /* 配置SDIO_DAT0(PC8), DAT1(PC9), DAT2(PC10), DAT3(PC11), CLK(PC12) 和 CMD(PD2) */
    gpio_af_set(GPIOC, GPIO_AF_12, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_af_set(GPIOD, GPIO_AF_12, GPIO_PIN_2);
    
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
    
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_12);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_12);
    
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_2);
}

/*!
    \brief      配置SDIO和DMA的时钟
    \param[in]  无
    \param[out] 无
    \retval     无
*/
static void rcu_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
	rcu_periph_clock_enable(RCU_GPIOE);
    
    rcu_periph_clock_enable(RCU_SDIO);
    rcu_periph_clock_enable(RCU_DMA1);
}

/*!
    \brief      配置DMA1通道3用于数据发送（内存到外设）
    \param[in]  srcbuf: 源缓冲区指针
    \param[in]  bufsize: 缓冲区大小（未使用）
    \param[out] 无
    \retval     无
*/
static void dma_transfer_config(uint32_t *srcbuf, uint32_t bufsize)
{
    dma_multi_data_parameter_struct dma_struct;
    /* 清除所有中断标志 */
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_FEE);
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_SDE);
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_TAE);
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_HTF);
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_FTF);
    dma_channel_disable(DMA1, DMA_CH3);
    dma_deinit(DMA1, DMA_CH3);
    
    /* 配置DMA1通道3 */
    dma_struct.periph_addr = (uint32_t)SDIO_FIFO_ADDR;
    dma_struct.memory0_addr = (uint32_t)srcbuf;
    dma_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_struct.number = 0;
    dma_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_struct.periph_width = DMA_PERIPH_WIDTH_32BIT;
    dma_struct.memory_width = DMA_MEMORY_WIDTH_32BIT; 
    dma_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_struct.periph_burst_width = DMA_PERIPH_BURST_4_BEAT;
    dma_struct.memory_burst_width = DMA_MEMORY_BURST_4_BEAT;
    dma_struct.critical_value = DMA_FIFO_4_WORD;
    dma_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_multi_data_mode_init(DMA1, DMA_CH3, &dma_struct);
    
    dma_flow_controller_config(DMA1, DMA_CH3, DMA_FLOW_CONTROLLER_PERI);
    dma_channel_subperipheral_select(DMA1, DMA_CH3, DMA_SUBPERI4);
    dma_channel_enable(DMA1, DMA_CH3);
}

/*!
    \brief      配置DMA1通道3用于数据接收（外设到内存）
    \param[in]  dstbuf: 目标缓冲区指针
    \param[in]  bufsize: 缓冲区大小（未使用）
    \param[out] 无
    \retval     无
*/
static void dma_receive_config(uint32_t *dstbuf, uint32_t bufsize)
{
    dma_multi_data_parameter_struct dma_struct;
    /* 清除所有中断标志 */
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_FEE);
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_SDE);
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_TAE);
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_HTF);
    dma_flag_clear(DMA1, DMA_CH3, DMA_FLAG_FTF);
    dma_channel_disable(DMA1, DMA_CH3);
    dma_deinit(DMA1, DMA_CH3);
    
    /* 配置DMA1通道3 */
    dma_struct.periph_addr = (uint32_t)SDIO_FIFO_ADDR;
    dma_struct.memory0_addr = (uint32_t)dstbuf;
    dma_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_struct.number = 0;
    dma_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_struct.periph_width = DMA_PERIPH_WIDTH_32BIT;
    dma_struct.memory_width = DMA_MEMORY_WIDTH_32BIT; 
    dma_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_struct.periph_burst_width = DMA_PERIPH_BURST_4_BEAT;
    dma_struct.memory_burst_width = DMA_MEMORY_BURST_4_BEAT;
    dma_struct.critical_value = DMA_FIFO_4_WORD;
    dma_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_multi_data_mode_init(DMA1, DMA_CH3, &dma_struct);
    
    dma_flow_controller_config(DMA1, DMA_CH3, DMA_FLOW_CONTROLLER_PERI);
    dma_channel_subperipheral_select(DMA1, DMA_CH3, DMA_SUBPERI4);
    dma_channel_enable(DMA1, DMA_CH3);
}

