/*
 * user.c
 *
 *  Created on: 2025年9月27日
 *      Author: qiu
 */
#include "user.h"
#include "rtc.h"

/* 用于获取时间 */
static rtc_time_t Get_Current_Time = {
    .tm_sec = 0,  // 秒
    .tm_min = 0,  // 分
    .tm_hour = 0, // 小时
    .tm_wday = 0, // 星期
    .tm_mday = 0, // 日
    .tm_mon = 0,  // 月
    .tm_year = 0  // 年份
};

void bsp_run(void)
{
    char time_str[33]={0};

    //UART0_Init();
    RTC_Init();

     while(1)
     {
        /* 判断RTC获取时间函数状态 */
        if (Rtc_GetTime(&Get_Current_Time))
        {
            /* 格式化并输出时间 */
            FormatRtcTime(&Get_Current_Time, time_str, sizeof(time_str));
            //printf("当前RTC时间: %s\r\n", time_str);
        }
     }
}

