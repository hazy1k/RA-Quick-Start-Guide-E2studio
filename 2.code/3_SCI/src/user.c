/*
 * user.c
 *
 *  Created on: 2025年9月24日
 *      Author: qiu
 */
#include "user.h"
#include "uart.h"

void bsp_run(void)
{
    uart0_init();
    printf("接下来开始串口循环实验，请输入回环内容：\r\n");
    while(1);
}

