/*
 * rtc.c
 *
 *  Created on: 2025年9月27日
 *      Author: qiu
 */
#include "rtc.h"

const char* Convet_tsr[7] = {"一","二","三","四","五","六","天"};

/* 中断标志位 */
bool Rtc_Flag_Interr = false;
static rtc_time_t Get_Current_Time;

/* 初始化时间设置 */
static  rtc_time_t Set_Init_Time = {
    .tm_sec = 40,       // 秒
    .tm_min = 59,      // 分
    .tm_hour = 23,     // 小时
    .tm_wday = 2,      // 星期
    .tm_mday = 15,     // 日
    .tm_mon = 7,       // 月
    .tm_year = 2025 - YEAR_FIXED  // 年份
};

/* RTC初始化函数 */
void RTC_Init(void)
{
    /* 初始化RTC */
    fsp_err_t err = R_RTC_Open(g_rtc0.p_ctrl, g_rtc0.p_cfg);
    if (FSP_SUCCESS != err) {
        //printf("错误：RTC初始化中断设置失败\n");
    }

    /* 设置RTC计时 */
        err = R_RTC_CalendarTimeSet(g_rtc0.p_ctrl, &Set_Init_Time);
        if (FSP_SUCCESS != err) {
           //printf("错误：RTC计时设置失败\n");
        }

    /* 使能计时中断 */
        err = R_RTC_PeriodicIrqRateSet(g_rtc0.p_ctrl, RTC_PERIODIC_IRQ_SELECT_1_SECOND);
        if (FSP_SUCCESS != err) {
            //printf("错误：RTC中断设置失败\n");
        }
}

/* RTC获取时间函数 */
bool Rtc_GetTime(rtc_time_t *Nne_time)
{
    if (Rtc_Flag_Interr)
    {
        /* 复制当前时间 */
        *Nne_time = Get_Current_Time;
        /* 清除中断标志位 */
        Rtc_Flag_Interr = false;
        return true;
    }
    return false;
}
/* RTC中断函数 */
void rtc_callback(rtc_callback_args_t *p_args)
{
    if (p_args->event = RTC_EVENT_PERIODIC_IRQ)
    {
        /* 获取当前时间 */
        fsp_err_t err = R_RTC_CalendarTimeGet(g_rtc0.p_ctrl, &Get_Current_Time);
        if (FSP_SUCCESS != err) {
            //printf("错误：获取时间失败\n");
        }

        /* 标记时间更新 */
        Rtc_Flag_Interr = true;
    }
}

/* 格式化RTC时间为字符串 */
void FormatRtcTime(const rtc_time_t *time, char *buffer, size_t size)
{

    int wday = time->tm_wday - 1;
    if (wday < 0 || wday > 7) wday = 0;  // 超出范围时默认取第一个

    if (time && buffer && size > 0) {
        /*snprintf(buffer, size, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d 星期:%s",
                 time->tm_year + YEAR_FIXED,
                 time->tm_mon,
                 time->tm_mday,
                 time->tm_hour,
                 time->tm_min,
                 time->tm_sec,
                 Convet_tsr[wday]
                 );
        */
    }
}
