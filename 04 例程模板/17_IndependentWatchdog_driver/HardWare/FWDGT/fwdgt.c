/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：fwdgt.c
 * 作者: Jialei Zhao
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2025/12/29      V0.01    original
************************************************************/

/************************* 头文件 *************************/
#include "fwdgt.h"

/************************* 宏定义 *************************/

/************************ 变量定义 ************************/

/************************ 函数定义 ************************/

/************************************************************ 
 * Function :       my_fwdgt_init
 * Comment  :       用于初始化独立看门狗
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void my_fwdgt_init(void) {

    /* confiure FWDGT counter clock: 32KHz(IRC32K) / 64 = 0.5 KHz */
    // 当FWDG启动时，FWDG时钟被强制选择由IRC32K时钟做为时钟源。
    //2 * 500  = 1000 / 0.5 = 2000ms
    fwdgt_config(2*500,FWDGT_PSC_DIV64);
    /* After 2 seconds to generate a reset */
    fwdgt_enable();
}

/************************************************************ 
 * Function :       my_fwdgt_feed
 * Comment  :       用于喂独立看门狗
 * Parameter:       null
 * Return   :       null
 * Author   :       Jialei Zhao
 * Date     :       2025-12-29 V0.1 original
************************************************************/
void my_fwdgt_feed(void){
    fwdgt_counter_reload();
}

/****************************End*****************************/
