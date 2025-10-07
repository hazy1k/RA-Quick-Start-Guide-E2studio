/*
 * agtpwm.c
 *
 *  Created on: 2025年9月24日
 *      Author: qiu
 */
#include "agtpwm.h"
#include "hal_data.h"

void pwm_init(void)
{
    R_AGT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    R_AGT_Start(&g_timer0_ctrl);
}

void set_pwm_cycle(uint8_t number)
{
    timer_info_t timer_info;
    uint64_t get_cycle_count;
    uint32_t counts;
    fsp_err_t err;

    // 限制占空比范围在0-100%
    number = (number > 100) ? 100 : number;

    // 获取定时器信息
    err = R_AGT_InfoGet(&g_timer0_ctrl, &timer_info);
    if (FSP_SUCCESS != err) {
        return;// 错误处理逻辑
    }

    // 获取定时器周期计数
    get_cycle_count = timer_info.period_counts;

    // 使用32位计算避免溢出
    counts = (uint32_t)((get_cycle_count * number) / 100);

    // 设置PWM占空比
    err = R_AGT_DutyCycleSet(&g_timer0_ctrl, counts, AGT_OUTPUT_PIN_AGTOA);
    if (FSP_SUCCESS != err) {
        return;// 错误处理逻辑
    }
}
