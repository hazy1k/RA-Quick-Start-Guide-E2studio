/*
 * user.c
 *
 *  Created on: 2025年9月25日
 *      Author: qiu
 */
#include "user.h"
#include "gptpwm.h"

void bsp_run(void)
{
    uint16_t count = 0;
    pwm_init();
    while(1)
    {
        for(count=0;count<100;count++)
        {
            pwm_set(count);
            R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
        }
    }

}
