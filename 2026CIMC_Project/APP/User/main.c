#include "main.h"
void main(void) {
    System_Init();
    while (1) {
        /* LED 心跳在中断中处理; 协议帧解析在 USART1 中断中 */
    }
}
