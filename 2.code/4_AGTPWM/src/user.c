/*
 * user.c
 *
 *  Created on: 2025年9月24日
 *      Author: qiu
 */
#include "user.h"
#include "agtpwm.h"

void bsp_run(void)
{
    uint8_t i=0;

    //初始化AGT
    pwm_init ();

    while (1)
    {
         i++;
         if(i>100) i=0;
         set_pwm_cycle(i);  //设置占空比
         R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
