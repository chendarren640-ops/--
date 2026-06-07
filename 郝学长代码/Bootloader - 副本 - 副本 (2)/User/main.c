/*
 * main.c
 * Bootloader 入口
 */

#include "HeaderFiles.h"
#include "Function.h"

int main(void)
{
    System_Init();
    UsrFunction();
    while(1) {} /* 不应到达这里 */
}
