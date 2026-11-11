# 参数逐项表

物理值=解码后的整数÷比例。int16使用二补码；0..41沿用唯一参数表，42..49原保护，50..56运行期配置。

|序号|待应用Modbus|实际Modbus|成员|类型|乘数|原始范围|
|---|---|---|---|---|---|---|
|0|`0x1A00`|`0x1A80`|`g_chb_dab_params.sample_period_s`|uint16|10000000.0|1..10000|
|1|`0x1A01`|`0x1A81`|`g_chb_dab_params.modulation_max`|uint16|10000.0|1..9800|
|2|`0x1A02`|`0x1A82`|`g_chb_dab_params.precharge_voltage_v`|uint16|10.0|0..30000|
|3|`0x1A03`|`0x1A83`|`g_chb_dab_params.precharge_stable_10ms`|uint16|1.0|1..65535|
|4|`0x1A04`|`0x1A84`|`g_chb_dab_params.precharge_timeout_10ms`|uint16|1.0|1..65535|
|5|`0x1A05`|`0x1A85`|`g_chb_dab_params.bypass_delay_10ms`|uint16|1.0|0..65535|
|6|`0x1A06`|`0x1A86`|`g_chb_dab_params.breaker_pulse_10ms`|uint16|1.0|1..65535|
|7|`0x1A07`|`0x1A87`|`g_chb_dab_params.pll_nominal_frequency_hz`|uint16|100.0|100..10000|
|8|`0x1A08`|`0x1A88`|`g_chb_dab_params.pll_kp`|uint16|10.0|0..65535|
|9|`0x1A09`|`0x1A89`|`g_chb_dab_params.pll_ki`|uint16|1.0|0..65535|
|10|`0x1A0A`|`0x1A8A`|`g_chb_dab_params.pll_correction_limit_rad_s`|uint16|100.0|0..65535|
|11|`0x1A0B`|`0x1A8B`|`g_chb_dab_params.pll_lock_error_pu`|uint16|10000.0|1..10000|
|12|`0x1A0C`|`0x1A8C`|`g_chb_dab_params.pll_min_grid_peak_v`|uint16|10.0|0..65535|
|13|`0x1A0D`|`0x1A8D`|`g_chb_dab_params.pll_lock_10ms`|uint16|1.0|1..65535|
|14|`0x1A0E`|`0x1A8E`|`g_chb_dab_params.pll_timeout_10ms`|uint16|1.0|1..65535|
|15|`0x1A0F`|`0x1A8F`|`g_chb_dab_params.voltage_kp`|int16 two-complement|100000.0|-32768..32767|
|16|`0x1A10`|`0x1A90`|`g_chb_dab_params.voltage_ki`|int16 two-complement|100.0|-32768..32767|
|17|`0x1A11`|`0x1A91`|`g_chb_dab_params.voltage_current_limit_a`|uint16|100.0|1..65535|
|18|`0x1A12`|`0x1A92`|`g_chb_dab_params.startup_strategy`|uint16|1.0|0..1|
|19|`0x1A13`|`0x1A93`|`g_chb_dab_params.startup_udc_offset_v`|int16 two-complement|100.0|-32768..32767|
|20|`0x1A14`|`0x1A94`|`g_chb_dab_params.startup_udc_step_v`|uint16|1000.0|1..65535|
|21|`0x1A15`|`0x1A95`|`g_chb_dab_params.startup_id_reference_a`|int16 two-complement|100.0|-32768..32767|
|22|`0x1A16`|`0x1A96`|`g_chb_dab_params.startup_id_ramp_step_a`|uint16|10000.0|1..65535|
|23|`0x1A17`|`0x1A97`|`g_chb_dab_params.startup_ramp_tad`|uint16|1.0|1..65535|
|24|`0x1A18`|`0x1A98`|`g_chb_dab_params.current_kp`|int16 two-complement|1000.0|-32768..32767|
|25|`0x1A19`|`0x1A99`|`g_chb_dab_params.current_ki`|int16 two-complement|1000.0|-32768..32767|
|26|`0x1A1A`|`0x1A9A`|`g_chb_dab_params.closed_loop_voltage_limit_factor`|uint16|10000.0|1..30000|
|27|`0x1A1B`|`0x1A9B`|`g_chb_dab_params.current_feedback_sign`|int16 two-complement|1.0|-1..1|
|28|`0x1A1C`|`0x1A9C`|`g_chb_dab_params.current_phase_compensation_rad`|int16 two-complement|10000.0|-31415..31415|
|29|`0x1A1D`|`0x1A9D`|`g_chb_dab_params.output_phase_compensation_rad`|int16 two-complement|10000.0|-31415..31415|
|30|`0x1A1E`|`0x1A9E`|`g_chb_dab_params.ac_overcurrent_trip_a`|uint16|100.0|1..65535|
|31|`0x1A1F`|`0x1A9F`|`g_chb_dab_params.dc_overvoltage_trip_v`|uint16|10.0|1..65535|
|32|`0x1A20`|`0x1AA0`|`g_chb_dab_params.protection_confirm_samples`|uint16|1.0|1..65535|
|33|`0x1A21`|`0x1AA1`|`g_chb_dab_params.dab_start_duty_count`|uint16|1.0|0..2500|
|34|`0x1A22`|`0x1AA2`|`g_chb_dab_params.dab_target_duty_count`|uint16|1.0|0..2500|
|35|`0x1A23`|`0x1AA3`|`g_chb_dab_params.dab_ramp_step_count`|uint16|1.0|1..2500|
|36|`0x1A24`|`0x1AA4`|`g_chb_dab_params.dab_start_count`|uint16|1.0|0..4999|
|37|`0x1A25`|`0x1AA5`|`g_chb_dab_command.ac_mode`|uint16|1.0|0..1|
|38|`0x1A26`|`0x1AA6`|`g_chb_dab_command.open_loop_modulation`|uint16|10000.0|0..9800|
|39|`0x1A27`|`0x1AA7`|`g_chb_dab_command.open_loop_frequency_hz`|uint16|100.0|100..10000|
|40|`0x1A28`|`0x1AA8`|`g_chb_dab_command.dc_voltage_ref_v`|uint16|10.0|1..65535|
|41|`0x1A29`|`0x1AA9`|`g_chb_dab_command.iq_reference_a`|int16 two-complement|100.0|-32768..32767|
|42|`0x1A2A`|`0x1AAA`|`OC_level_SET`|uint16|1|0..65535|
|43|`0x1A2B`|`0x1AAB`|`OC_time_SET`|uint16|1|0..65534|
|44|`0x1A2C`|`0x1AAC`|`OC_level_SET_L2`|uint16|1|0..65535|
|45|`0x1A2D`|`0x1AAD`|`OC_time_SET_L2`|uint16|1|0..65534|
|46|`0x1A2E`|`0x1AAE`|`OV_level_SET`|uint16|1|0..65535|
|47|`0x1A2F`|`0x1AAF`|`OV_time_SET`|uint16|1|0..65534|
|48|`0x1A30`|`0x1AB0`|`HA_L_Voltage_Precharge`|uint16|10|0..65535|
|49|`0x1A31`|`0x1AB1`|`HA_H_Voltage`|uint16|10|0..65535|
|50|`0x1A32`|`0x1AB2`|`g_chb_dab_protection_enable`|uint16 bitmap|1|0..65535|
|51|`0x1A33`|`0x1AB3`|`system_mode_HVAC`|uint16 bitmap|1|0..65535|
|52|`0x1A34`|`0x1AB4`|`Port_AD_PORT_EN`|uint16 bitmap|1|0..65535|
|53|`0x1A35`|`0x1AB5`|`Port_FO_1_EN`|uint16 bitmap|1|0..65535|
|54|`0x1A36`|`0x1AB6`|`Port_FO_2_EN`|uint16 bitmap|1|0..65535|
|55|`0x1A37`|`0x1AB7`|`Port_FO_3_EN`|uint16 bitmap|1|0..65535|
|56|`0x1A38`|`0x1AB8`|`Port_FB_EN`|uint16 bitmap|1|0..65535|

表内范围之外还检查sample_period不变、DAB起始≤目标、开环调制≤最大值、电流反馈符号仅±1、巡检低压<高压；参数0硬件采样周期只读。
AD只允许F000，FO仅7FFF，系统方式仅0..3，快速保护仅0/1。未分配待应用/实际区剩余71字保留，不得写入。
