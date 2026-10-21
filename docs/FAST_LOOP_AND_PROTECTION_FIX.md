# 2026-10-21 快速环与保护修正

适用分支：`refactor/ae6e512-control-cleanup`。本次未修改默认 PI 数值、启动给定或预充时序。

## PI 配置与快速执行

`Chb_ConfigureControllers()` 在初始化和进入 RUNNING 前配置电压、d 轴和 q 轴 PI。
固定的 Kp、Ki、Ts 不再在每个 100 μs 周期重新配置和校验；停机修改后，下一次启动重新应用。
`SstPi_UpdateBounds()` 只更新动态输出/积分边界并钳位已有积分。
全量 `Chb_ConfigurationValid()` 留在启动及通信参数提交入口，不在 FastStep 执行。
运行周期保留必要的采样异常、计算溢出、实际母线和故障检查，删除重复的有限数检查。

## 单相最终输出与抗饱和

单相不再用虚拟 beta 的幅值限制整个 dq 矢量，也不执行电流环 dq 圆开方。
d/q 分别计算 P/I，加上电网前馈后，用同一输出相角逆变换得到物理 A 相电压。
`phase_a` 按实测总母线乘有效调制上限限幅；最终写入前仍保留物理 Clamp。

本拍试算的限幅偏差为 `delta = phase_limited - phase_command`，对两轴的投影为：

```text
d 轴：delta * cos(theta)
q 轴：-delta * sin(theta)
```

若投影偏差与本拍实际积分增量反向，则撤销该轴本拍积分，并重新合成输出；反向恢复积分允许继续。
该判断使用包含前馈的实际电压命令，因此启动母线不足时也能识别饱和。
积分绝对边界仍为可用母线电压的两倍。三相保留前馈参与的 dq 圆限幅和抗饱和，仅超限时开方。

## 唯一直流过压保护

当前 CHB 软件直流过压只使用：

- 门限：`g_chb_dab_params.dc_overvoltage_trip_v`，单位 V/模块。
- 确认：`g_chb_dab_params.protection_confirm_samples` 个连续 TAD 样本，默认 5 拍。
- 反馈：单相取 `SVG_Udc_A_Ave`；三相取 A/B/C 相模块均压的最大值。
- 结果：`CHB_FAULT_DC_OVERVOLTAGE`，控制故障码 4；故障复位沿用原锁存流程。

移除快速链中的 `Udc_check()` 和旧 HVAC 滤波直流过压判据。
旧 `OV_level_SET`、`OV_time_SET`、`HA_H_Voltage` 等字段保留地址及持久化兼容，当前 CHB 不再消费为第二套直流保护。
瞬时交流过流和原一级/二级 RMS 过流保留；原 RMS 计时、锁存及下一拍消费顺序保持。
硬件故障入口保持。该均压保护不代表相内每一个模块都低于门限。

LOW=1 默认模块参考为 60 V，直流过压为 75 V；LOW=0 仍是历史 680 V 模块参数组。
两者都不是自动选择的 380 Vrms/132 V 模块配置。改变试验工作点时，应在停机状态核对电压参考、预充及唯一直流过压参数，不能只改 PI 或低压宏。
软件保护总使能 `g_chb_dab_protection_enable` 沿用原逻辑，置零会关闭这些软件判据。

## 验证

使用临时 Zig 0.13.0 的 Clang C 前端编译生产控制源码和主机替代 I/O：

- `tests/pi_regression.py`：PI 正负方向、动态边界、饱和、跟踪、数值异常。
- `tests/common_control_safety.py`：三相、单相高压、单相低压及默认组合；五个相角各 10000 拍的饱和保持、反向积分恢复、虚拟 beta 不压低 A 相、固定增益缓存及启动入口重新配置。
- `tests/single_phase_control.py`：SOGI/PLL、启动接管、实测母线归一化、最终 A 相限幅、模式切换和故障竞态。
- `tests/protection_regression.py`：编译实际 HVAC 与快速保护函数体，验证唯一过压门限/连续计数、相选择、瞬时过流及两级 RMS 过流锁存。
- `tests/realtime_audit.py`：81/81 静态不变量通过；旧保护变更使用精确前后哈希合同核对。
- 注释覆盖和源码迁移合同检查通过。

本次没有 TI C6748 工具链构建结果，也没有实板闭环与 ISR 宽度测量。主机测试不证明 380 V 启动无过冲，或中断已满足 100 μs 最坏执行时间。
