/*
 * adc.c
 *
 *  Created on: 2025年9月25日
 *      Author: qiu
 */
#include "adc.h"
#include "r_adc_api.h"

volatile bool adc_flag = false;

void adc_init(void)
{
    fsp_err_t err;

    // 打开ADC设备
    err = R_ADC_Open(&g_adc0_ctrl, &g_adc0_cfg);
    if (FSP_SUCCESS != err) {
        //printf("ADC初始化失败! \n");
        return;
    }

    // 配置ADC扫描
    err = R_ADC_ScanCfg(&g_adc0_ctrl, &g_adc0_channel_cfg);
    if (FSP_SUCCESS != err) {
        //printf("ADC扫描配置失败! \n");
        R_ADC_Close(&g_adc0_ctrl);  // 关闭已打开的ADC
        return;
    }

    //printf("ADC初始化成功!\n");
}

void adc_callback(adc_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    adc_flag = true;
}

double adc_read_value(void)
{
    uint16_t adc_data = 0;
    double temperature = 0.0F;

    // 启动ADC扫描
    fsp_err_t err = R_ADC_ScanStart(&g_adc0_ctrl);
    if (FSP_SUCCESS != err) {
        //printf("ADC扫描启动失败! \n");
        return 9999;  // 返回错误值
    }

    // 等待ADC转换完成
    while (!adc_flag);
    adc_flag = false;  // 清除标志位

    // 读取ADC数据
    err = R_ADC_Read(&g_adc0_ctrl, ADC_CHANNEL_0, &adc_data);
    if (FSP_SUCCESS != err) {
        //printf("ADC数据读取失败!\n");
        return 9999;  // 返回错误值
    }

    // 计算温度值
    temperature = (adc_data * 3.3)/ 4096;

    return temperature;
}
