/*
 * gpio.c
 *
 *  Created on: 2025年9月22日
 *      Author: qiu
 */

#include "led.h"

void led_run(void)
{
    while(1)
    {
        R_IOPORT_PinWrite(&g_ioport_ctrl,BSP_IO_PORT_04_PIN_02,BSP_IO_LEVEL_LOW);
        R_BSP_SoftwareDelay (1, BSP_DELAY_UNITS_SECONDS);

        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_04_PIN_02, BSP_IO_LEVEL_HIGH);
        R_BSP_SoftwareDelay (1, BSP_DELAY_UNITS_SECONDS);
    }
}
