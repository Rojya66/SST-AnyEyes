# 100 Vac 专用宏与低压试验程序移除记录

日期：2026-11-16。工程：`D:/Code/SST/AnyEyesVersion_DpramCompat`。

## 修改范围

本次按要求删除100 Vac专用配置，采用修改前 `SST_LOW_VOLTAGE_100V_TEST=0` 对应的非低压分支。不是把宏永久定义为0，也没有把低压数值改名后继续保留。

- `ChbBuildConfig.h`：删除 `SST_LOW_VOLTAGE_100V_TEST`、全部 `SST_LV100_*` 参数及相关组合检查。
- `ChbDabControl.c`：删除低压命令/参数初始化、启动/运行动态限流、低压dq电流预算和已注释的低压残留代码，直接保留原非低压分支。
- `ControlDefaults.c`：删除低压对原电压门槛、目标及电流上限的覆盖。
- `ProtectionConfig.h`：删除低压OC/OV默认阈值分支，保留原非低压值。
- `DpramDefaults.c`、`DPRAM.C`：删除低压默认给定和旧遥调限值分支；DPRAM屏幕适配器的地址、有效位、PI编码、启停及DAB独立控制保持原实现。
- 测试/构建脚本：移除低压变体和已失效的编译参数；单相、三相分别验证。开关测试补齐主机替身缺失的 `FaultTypeFLAG` 声明，不修改固件故障逻辑。
- 文档：更新当前配置、保护默认值及屏幕状态位解释。低压历史说明增加失效提示，历史证据JSON不改写。VOFA现行说明为 `docs/VOFA_TELEMETRY.md`。

单相AN采样、双SOGI、公共PI、开环/闭环选择、仿真器模式、AnyEyes/VOFA、DPRAM及开关输出/反馈宏继续保留。FPGA 10kHz中断入口、采样周期和串口调度不在本次修改范围。

## 恢复后的关键默认值

下表是非低压分支原有数值的恢复，不是新整定的一组现场参数。原低压分支中后续试验改过的133 V给定、25 V核心预充等值随分支一起移除。

| 对象 | 参数 | 当前默认值 |
|---|---|---:|
| `g_chb_dab_command` | `run_enable` / `dab_enable` | 0 / 0，上电停机 |
| 同上 | `ac_mode` | `CHB_AC_MODE_OPEN_LOOP`，闭环需显式选择 |
| 同上 | `dc_voltage_ref_v` | 680 V/模块 |
| `g_chb_dab_params` | `precharge_voltage_v` | 280 V/模块 |
| 同上 | `voltage_kp` / `voltage_ki` | 0.099375 / 12.5 |
| 同上 | `voltage_current_limit_a` | 95 A峰值，约束外环输出/启动Id |
| 同上 | `startup_strategy` | `CHB_STARTUP_SOURCE_COMPATIBLE` |
| 同上 | `startup_udc_offset_v` | 20 V/模块 |
| 同上 | `startup_udc_step_v` | 2 V/模块/TAD |
| 同上 | `startup_id_reference_a` / `startup_id_ramp_step_a` | 10 A / 0.05 A每TAD |
| 同上 | `current_kp` / `current_ki` | -1.249166667 / -16.66666667，保留负PI方向 |
| 同上 | `ac_overcurrent_trip_a` / `dc_overvoltage_trip_v` | 120 A瞬时 / 780 V模块均压 |
| 同上 | `protection_confirm_samples` | 5 TAD |
| 原软件保护 | 一级/二级OC默认值 | 160 / 180 A有效值 |
| 原软件保护 | OV默认值 | 830 V，比较模块均压合成滤波值 |
| 原10kV初始化分支 | `HA_L_Voltage_Precharge` / `HA_H_Voltage` | 350 / 780 V，原逐模块巡检阈值 |
| 原遥调初始化 | `PORT_V_SET` / `Idst_Max` | 680 V / 95 A |

核心 `precharge_voltage_v` 与旧巡检 `HA_L_Voltage_Precharge` 是两个不同参数；本次分别恢复原值，不强制合并。原软件保护与快速软件保护的阈值也不自动同步。

旧 `DPRAM.C` 通用遥调分支恢复600..750 V电压给定范围（越界回落700 V）、10..95 A电流上限范围。当前屏幕实际写入仍经 `SstDpramParameter` / `SstParameterTable` 校验，未将这些旧限制硬套到现代参数接口。

`COM_SYS_MODE.HA_RunMode` 恢复1，只表示“运行/测试”管理方式；它不是 `run_enable`，不会自动启动PWM。当前配置仍为单相1、真实采样、DPRAM1、仿真器0、VOFA、460800 bit/s，开关输出/反馈0，EEPROM上电自动加载0。

## 兼容保留字段

为了保留 `CHB_DAB_STATUS` 布局和屏幕状态字位置：

- `load_ready`、`load_ready_count` 保留为固定0字段，不再运行就绪确认或扩大电流限幅的程序。
- Modbus `0x198F` 的bit3保持原位并标为保留固定0；bit0/1/2/4仍分别为CHB PWM、DAB PWM、PLL锁定和外环生效。
- `current_limit_active_a` 只保留复位/启动时的兼容镜像；运行实际限幅读取 `g_chb_dab_params.voltage_current_limit_a`。

VOFA十二通道、AnyEyes地址及DPRAM参数/故障点表未重新编号。

## 验证结果与边界

- CCS默认单相工程：Debug、Release均清理编译成功，报告为 `build/ccs_validation/result.json`。
- TI C6000三相Release：`SST_PHASE_A_ONLY_TEST=0`、AnyEyes、开关输出及反馈均启用，95个源文件编译及链接成功，报告为 `build/Release/no_lv_three_phase/build_result.json`。
- 两种构建的固件输入SHA256一致：`db1a6b1a7d4b263597182f352ce3ff3a93ff15cc05ca9a48c1697224355b3e54`，构建期间未变化。
- 对 `ChbDabControl.c`、`ControlDefaults.c`、`ProtectionConfig.h`、`DpramDefaults.c`、`DPRAM.C` 做可执行token对比：本次结果与修改前非低压分支完全一致，报告为 `build/lv100_removal/non_low_branch_equivalence.json`。
- 通过：`control_regression`、`dpram_screen`、`dpram_commands`、`dpram_maintenance`、`dpram_host_reply`、`dpram_switch_maintenance`、`single_phase_io`、`vofa_telemetry`、`pi_regression`。`switch_io` 补齐测试替身后，单/三相与20种输出/反馈/线圈极性组合全部通过。
- 固件源码内没有残留 `SST_LV100_*` 或低压模式宏引用；`git diff --check` 通过。

**已有未通过项：** `tests/common_control_safety.py` 在修改前、修改后均在运行中切换模式的故障断言处失败。当前现场源码将 `ChbDabControl_Service()` 中模式变化后的 `CHB_FAULT_BAD_CONFIGURATION` 调用及返回注释掉；本次未启用或改写这些既有逻辑。因此不能把本次编译/专项测试通过理解为全部公共安全检查已通过。未做实际功率回路试验或板上100us执行时间测量。

历史验证工具中为读取旧源码保留的低压分支识别字符串、归档文档和证据不参与当前固件的低压配置；不应再运行旧低压构建命令作为现行构建方式。
