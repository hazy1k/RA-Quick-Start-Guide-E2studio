/*
 * gptpwm.c
 *
 *  Created on: 2025年9月25日
 *      Author: qiu
 */
#include "gptpwm.h"

void pwm_init(void)
{
    /* 初始化 GPT 模块 */
    R_GPT_Open(&g_timer3_ctrl, &g_timer3_cfg);
    /* 设置定时器周期为5000个时钟单位*/
    R_GPT_PeriodSet(&g_timer3_ctrl, 50000);
    //  /* 重置定时器计数器，确保从0开始计数 */
    R_GPT_Reset(&g_timer3_ctrl);
    /* 启动 GPT 定时器 */
    R_GPT_Start(&g_timer3_ctrl);
    /* 初始化 GPT 模块 */
    R_GPT_Open(&g_timer3_ctrl, &g_timer3_cfg);
    /* 启动 GPT 定时器 */
    R_GPT_Start(&g_timer3_ctrl);
}

void pwm_set(uint16_t value)
{
    timer_info_t info;
    uint32_t period_counts;

    /*获取GPT3信息*/
    R_GPT_InfoGet(&g_timer3_ctrl, &info);

    period_counts = info.period_counts;

    /*设置PWM周期*/
    R_GPT_DutyCycleSet(&g_timer3_ctrl,
                (uint32_t)((period_counts * value) / 100),
                GPT_IO_PIN_GTIOCA
            );
}

/* GPT定时器 中断回调函数 */
void gpt3_callback(timer_callback_args_t *p_args)
{
    if (p_args->event == TIMER_EVENT_CYCLE_END)
    {
        R_PORT4->PODR ^= 1<<(BSP_IO_PORT_04_PIN_02 & 0xFF);
    }
}
