/*
 * user.c
 *
 *  Created on: 2025年9月27日
 *      Author: qiu
 */
#include "user.h"
#include "dac.h"

void bsp_run(void)
{
    uint16_t dac_value = 0;
    float voltage = 0.0f;
    dac_init();
    while(1)
    {
        if(voltage>3.3)
        {
            voltage=0.0;
        }

        //printf("输出电压=%.2f\r\n",voltage);
        dac_value = voltage_transition(voltage);
        dac_set(dac_value);
        voltage=voltage+0.1f;
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
}

