/**
 * @file    main.c
 * @brief   CIMC 2026 初赛 APP 主程序
 *
 * 系统启动流程:
 *   System_Init() -> 初始化所有外设
 *   UsrFunction() -> 进入业务主循环
 */

#include "HeaderFiles.h"

int main(void)
{
    System_Init();
    UsrFunction();
}
