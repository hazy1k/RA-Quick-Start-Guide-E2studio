/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_iic_b_master.h"
#include "r_i2c_master_api.h"
FSP_HEADER
/* I2C Master on IIC Instance. */
extern const i2c_master_instance_t I2C_BH1750;

/** Access the I2C Master instance using these structures when calling API functions directly (::p_api is not used). */
extern iic_b_master_instance_ctrl_t I2C_BH1750_ctrl;
extern const i2c_master_cfg_t I2C_BH1750_cfg;

#ifndef BH1750_callback
void BH1750_callback(i2c_master_callback_args_t *p_args);
#endif
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
