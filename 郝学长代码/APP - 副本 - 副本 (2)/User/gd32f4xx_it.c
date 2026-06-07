/*!
    \file    gd32f4xx_it.c
    \brief   interrupt service routines

    \version 2020-09-04, V2.0.0, demo for GD32F4xx
*/

#include "gd32f4xx_it.h"
#include "systick.h"
#include "protocol_cmd.h"
#include "bsp_usart_rs485.h"

/* 串口接收中断处理 (将每个字节传递给协议层) */
void BSP_RS485_IRQHandler(void)
{
    uint8_t data;

    if (RESET != usart_interrupt_flag_get(BSP_RS485_USART, USART_INT_FLAG_RBNE))
    {
        data = (uint8_t)usart_data_receive(BSP_RS485_USART);
        Protocol_ReceiveByte(data);
    }

    if (RESET != usart_interrupt_flag_get(BSP_RS485_USART, USART_INT_FLAG_IDLE))
    {
        volatile uint32_t tmp;
        tmp = USART_STAT0(BSP_RS485_USART);
        tmp = USART_DATA(BSP_RS485_USART);
        (void)tmp;
    }
}

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    while (1){
    }
}

void MemManage_Handler(void)
{
    while (1){
    }
}

void BusFault_Handler(void)
{
    while (1){
    }
}

void UsageFault_Handler(void)
{
    while (1){
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    delay_decrement();
}
