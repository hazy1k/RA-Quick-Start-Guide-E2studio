/* generated HAL source file - do not edit */
#include "hal_data.h"
dac_instance_ctrl_t dac0_ctrl;
const dac_extended_cfg_t dac0_ext_cfg =
{ .enable_charge_pump = 0,
  .data_format = DAC_DATA_FORMAT_FLUSH_RIGHT,
  .output_amplifier_enabled = 0,
  .internal_output_enabled = false,
  .ref_volt_sel = (dac_ref_volt_sel_t) (0) };
const dac_cfg_t dac0_cfg =
{ .channel = 0, .ad_da_synchronized = false, .p_extend = &dac0_ext_cfg };
/* Instance structure to use this module. */
const dac_instance_t dac0 =
{ .p_ctrl = &dac0_ctrl, .p_cfg = &dac0_cfg, .p_api = &g_dac_on_dac };
void g_hal_init(void)
{
    g_common_init ();
}
