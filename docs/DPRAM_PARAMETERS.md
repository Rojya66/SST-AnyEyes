# DPRAM现代参数点表

生成源：`tools/dpram_point_table.py`；版本1，共54点。

所有读地址为DSP侧16位字。回复区参数地址是只读回读镜像，不能直接写入；参数写入通过请求区提交：word3=1，word4（点号）和word5（raw），补齐CRC和提交标记，最后写左邮箱21。回读位于回复区word32+点号，须按版本戳读取。详见 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)。

`physical = raw / scale`；int16按二补码解释。R/request-W表示主机读实际值、通过请求写参数；只有sample_period_s只读。启停/复位按控制源和存储互锁；其余写入须完全停机。

| 点号 | 实际回读地址 | 名称 | 类型 | scale/单位 | raw范围 | 权限 |
|---:|---|---|---|---|---|---|
| 0 | `0x66002D40` | `run_enable` | uint16 | 1 / bool | 0..1 | R / request-W |
| 1 | `0x66002D42` | `dab_enable` | uint16 | 1 / bool | 0..1 | R / request-W |
| 2 | `0x66002D44` | `ac_mode` | uint16 | 1 / enum | 0..1 | R / request-W |
| 3 | `0x66002D46` | `fault_reset` | uint16 | 1 / bool | 0..1 | R / request-W |
| 4 | `0x66002D48` | `open_loop_modulation` | uint16 | 10000 / pu | 0..9800 | R / request-W |
| 5 | `0x66002D4A` | `open_loop_frequency_hz` | uint16 | 100 / Hz | 100..10000 | R / request-W |
| 6 | `0x66002D4C` | `dc_voltage_ref_v` | uint16 | 10 / V | 1..65535 | R / request-W |
| 7 | `0x66002D4E` | `iq_reference_a` | int16 | 100 / A | -32768..32767 | R / request-W |
| 8 | `0x66002D50` | `g_chb_dab_protection_enable` | uint16 | 1 / bool | 0..1 | R / request-W |
| 9 | `0x66002D52` | `sample_period_s` | uint16 | 10000000 / s | 1..10000 | R |
| 10 | `0x66002D54` | `modulation_max` | uint16 | 10000 / pu | 1..9800 | R / request-W |
| 11 | `0x66002D56` | `precharge_voltage_v` | uint16 | 10 / V | 0..30000 | R / request-W |
| 12 | `0x66002D58` | `precharge_stable_10ms` | uint16 | 1 / 10ms | 1..65535 | R / request-W |
| 13 | `0x66002D5A` | `precharge_timeout_10ms` | uint16 | 1 / 10ms | 1..65535 | R / request-W |
| 14 | `0x66002D5C` | `bypass_delay_10ms` | uint16 | 1 / 10ms | 0..65535 | R / request-W |
| 15 | `0x66002D5E` | `breaker_pulse_10ms` | uint16 | 1 / 10ms | 1..65535 | R / request-W |
| 16 | `0x66002D60` | `pll_nominal_frequency_hz` | uint16 | 100 / Hz | 100..10000 | R / request-W |
| 17 | `0x66002D62` | `pll_kp` | uint16 | 10 /  | 0..65535 | R / request-W |
| 18 | `0x66002D64` | `pll_ki` | uint16 | 1 /  | 0..65535 | R / request-W |
| 19 | `0x66002D66` | `pll_correction_limit_rad_s` | uint16 | 100 / rad/s | 0..65535 | R / request-W |
| 20 | `0x66002D68` | `pll_lock_error_pu` | uint16 | 10000 / pu | 1..10000 | R / request-W |
| 21 | `0x66002D6A` | `pll_min_grid_peak_v` | uint16 | 10 / V | 0..65535 | R / request-W |
| 22 | `0x66002D6C` | `pll_lock_10ms` | uint16 | 1 / 10ms | 1..65535 | R / request-W |
| 23 | `0x66002D6E` | `pll_timeout_10ms` | uint16 | 1 / 10ms | 1..65535 | R / request-W |
| 24 | `0x66002D70` | `voltage_kp` | int16 | 100000 /  | -32768..32767 | R / request-W |
| 25 | `0x66002D72` | `voltage_ki` | int16 | 100 /  | -32768..32767 | R / request-W |
| 26 | `0x66002D74` | `voltage_current_limit_a` | uint16 | 100 / A | 1..65535 | R / request-W |
| 27 | `0x66002D76` | `startup_strategy` | uint16 | 1 / enum | 0..1 | R / request-W |
| 28 | `0x66002D78` | `startup_udc_offset_v` | int16 | 100 / V | -32768..32767 | R / request-W |
| 29 | `0x66002D7A` | `startup_udc_step_v` | uint16 | 1000 / V/TAD | 1..65535 | R / request-W |
| 30 | `0x66002D7C` | `startup_id_reference_a` | int16 | 100 / A | -32768..32767 | R / request-W |
| 31 | `0x66002D7E` | `startup_id_ramp_step_a` | uint16 | 10000 / A/TAD | 1..65535 | R / request-W |
| 32 | `0x66002D80` | `startup_ramp_tad` | uint16 | 1 / TAD | 1..65535 | R / request-W |
| 33 | `0x66002D82` | `current_kp` | int16 | 1000 /  | -32768..32767 | R / request-W |
| 34 | `0x66002D84` | `current_ki` | int16 | 1000 /  | -32768..32767 | R / request-W |
| 35 | `0x66002D86` | `closed_loop_voltage_limit_factor` | uint16 | 10000 / pu | 1..30000 | R / request-W |
| 36 | `0x66002D88` | `current_feedback_sign` | int16 | 1 /  | -1..1 | R / request-W |
| 37 | `0x66002D8A` | `current_phase_compensation_rad` | int16 | 10000 / rad | -31415..31415 | R / request-W |
| 38 | `0x66002D8C` | `output_phase_compensation_rad` | int16 | 10000 / rad | -31415..31415 | R / request-W |
| 39 | `0x66002D8E` | `ac_overcurrent_trip_a` | uint16 | 100 / A | 1..65535 | R / request-W |
| 40 | `0x66002D90` | `dc_overvoltage_trip_v` | uint16 | 10 / V | 1..65535 | R / request-W |
| 41 | `0x66002D92` | `protection_confirm_samples` | uint16 | 1 / samples | 1..65535 | R / request-W |
| 42 | `0x66002D94` | `dab_start_duty_count` | uint16 | 1 / count | 0..2500 | R / request-W |
| 43 | `0x66002D96` | `dab_target_duty_count` | uint16 | 1 / count | 0..2500 | R / request-W |
| 44 | `0x66002D98` | `dab_ramp_step_count` | uint16 | 1 / count/TAD | 1..2500 | R / request-W |
| 45 | `0x66002D9A` | `dab_start_count` | uint16 | 1 / count | 0..4999 | R / request-W |
| 46 | `0x66002D9C` | `OC_level_SET` | uint16 | 1 / A | 1..65535 | R / request-W |
| 47 | `0x66002D9E` | `OC_time_SET` | uint16 | 1 / 10ms counter threshold | 0..65534 | R / request-W |
| 48 | `0x66002DA0` | `OC_level_SET_L2` | uint16 | 1 / A | 1..65535 | R / request-W |
| 49 | `0x66002DA2` | `OC_time_SET_L2` | uint16 | 1 / 10ms counter threshold | 0..65534 | R / request-W |
| 50 | `0x66002DA4` | `OV_level_SET` | uint16 | 1 / V | 1..65535 | R / request-W |
| 51 | `0x66002DA6` | `OV_time_SET` | uint16 | 1 / 10ms counter threshold | 0..65534 | R / request-W |
| 52 | `0x66002DA8` | `HA_L_Voltage_Precharge` | uint16 | 10 / V | 0..65534 | R / request-W |
| 53 | `0x66002DAA` | `HA_H_Voltage` | uint16 | 10 / V | 1..65535 | R / request-W |

跨参数约束：开环调制≤modulation_max；current_feedback_sign只允许+1/-1（raw=1/FFFF）；DAB起始占空≤目标占空；HA_L_Voltage_Precharge<HA_H_Voltage。旧软件保护计时使用计数严格大于阈值的原判据，不能把计数阈值直接解释成精确跳闸延时。

v2保存37个现代参数（硬件TAD仅校验）、模式/开环调制/频率/DC给定/Iq给定以及8个原保护/巡检值。启停、复位、快速保护总使能不保存；v1加载只恢复原42字，不改8个新增原参数。
