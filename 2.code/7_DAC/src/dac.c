#include "dac.h"

void dac_set(uint16_t value)
{
    fsp_err_t err = R_DAC_Write(&dac0_ctrl,value);
    if(err != FSP_SUCCESS)
    {
        //printf("DAC通达打开失败\r\n");
    }
}

void dac_init(void)
{
    fsp_err_t err = R_DAC_Open(&dac0_ctrl,&dac0_cfg);
    if (err != FSP_SUCCESS)
    {
        //printf("DAC初始化失败\r\n");
     }
    err = R_DAC_Start(&dac0_ctrl);
    if (err != FSP_SUCCESS)
    {
        //printf("DAC启动失败\r\n");
        R_DAC_Close(&dac0_ctrl); // 关闭已打开的资源
     }
}

uint16_t voltage_transition(float value)
{
    return (uint16_t)((value/3.3f)*4095);
}
