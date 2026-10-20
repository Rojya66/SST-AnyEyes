# AnyEyes 与 SST 地址映射

本工程直接移植 F28377S_Plat 的 `C_COMM2PC` 命令编号、字节帧布局、功能码组/组内索引、200 个显示索引和最多16通道示波器机制。C6748 使用固定宽度 `uint16_t`，每个线上 octet 单独编码，不能把 C28x 的 `unsigned int` 或结构体内存当串口字节发送。

硬件/保护/快速控制算法未通过本映射改变。调试器仍可直接访问原始 float 成员；AnyEyes 的16位值是比例换算后的视图，会量化和饱和。地址范围始终为组0..29、组内0..33。未分配功能码读0、写拒绝；未分配显示索引读0。以下给出映射的唯一具体定义。

## 功能码

`F0` 请求 payload = group, index；`70` 响应 = group,index,value_hi,value_lo,result。result=0成功、1失败。`F1` 请求 payload = group,index,value_hi,value_lo；`71` 响应单字节0成功，错误1地址、2只读、3范围、4忙/运行中、5控制未授权、6存储失败。

浮点量的编码为 round(物理量 × scale)，解码为 raw / scale。标注 int16 的值需按16位二进制补码显示，例如 current_kp=-1.5 对应 raw=-1500，线上 FA 24。范围列为**编码值**范围。显示超范围会饱和，不截断回绕；写入超范围拒绝。

| 组/索引 | SST成员 | 编码 | scale | 单位 | 编码范围 | 写权限 |
|---|---|---|---:|---|---|---|
| 0/0 | `run_enable` | uint16 | 1 | bool | 0..1 | 串口控制授权 |
| 0/1 | `dab_enable` | uint16 | 1 | bool | 0..1 | 串口控制授权 |
| 0/2 | `ac_mode` | uint16 | 1 | enum | 0..1 | 停机写 |
| 0/3 | `fault_reset` | uint16 | 1 | bool | 0..1 | 串口控制授权 |
| 0/4 | `open_loop_modulation` | uint16 | 10000 | pu | 0..9800 | 停机写 |
| 0/5 | `open_loop_frequency_hz` | uint16 | 100 | Hz | 100..10000 | 停机写 |
| 0/6 | `dc_voltage_ref_v` | uint16 | 10 | V | 1..65535 | 停机写 |
| 0/7 | `iq_reference_a` | int16 | 100 | A | -32768..32767 | 停机写 |
| 0/8 | `g_chb_dab_protection_enable` | uint16 | 1 | bool | 0..1 | 停机写；仅控制现有软件保护开关 |
| 1/0 | `sample_period_s` | uint16 | 10000000 | s | 1..10000 | 只读（硬件TAD） |
| 1/1 | `modulation_max` | uint16 | 10000 | pu | 1..9800 | 停机写 |
| 1/2 | `precharge_voltage_v` | uint16 | 10 | V | 0..30000 | 停机写 |
| 1/3 | `precharge_stable_10ms` | uint16 | 1 | 10ms | 1..65535 | 停机写 |
| 1/4 | `precharge_timeout_10ms` | uint16 | 1 | 10ms | 1..65535 | 停机写 |
| 1/5 | `bypass_delay_10ms` | uint16 | 1 | 10ms | 0..65535 | 停机写 |
| 1/6 | `breaker_pulse_10ms` | uint16 | 1 | 10ms | 1..65535 | 停机写 |
| 1/7 | `pll_nominal_frequency_hz` | uint16 | 100 | Hz | 100..10000 | 停机写 |
| 1/8 | `pll_kp` | uint16 | 10 | — | 0..65535 | 停机写 |
| 1/9 | `pll_ki` | uint16 | 1 | — | 0..65535 | 停机写 |
| 1/10 | `pll_correction_limit_rad_s` | uint16 | 100 | rad/s | 0..65535 | 停机写 |
| 1/11 | `pll_lock_error_pu` | uint16 | 10000 | pu | 1..10000 | 停机写 |
| 1/12 | `pll_min_grid_peak_v` | uint16 | 10 | V | 0..65535 | 停机写 |
| 1/13 | `pll_lock_10ms` | uint16 | 1 | 10ms | 1..65535 | 停机写 |
| 1/14 | `pll_timeout_10ms` | uint16 | 1 | 10ms | 1..65535 | 停机写 |
| 1/15 | `voltage_kp` | int16 | 100000 | — | -32768..32767 | 停机写 |
| 1/16 | `voltage_ki` | int16 | 100 | — | -32768..32767 | 停机写 |
| 1/17 | `voltage_current_limit_a` | uint16 | 100 | A | 1..65535 | 停机写 |
| 1/18 | `startup_strategy` | uint16 | 1 | enum | 0..1 | 停机写 |
| 1/19 | `startup_udc_offset_v` | int16 | 100 | V | -32768..32767 | 停机写 |
| 1/20 | `startup_udc_step_v` | uint16 | 1000 | V/TAD | 1..65535 | 停机写 |
| 1/21 | `startup_id_reference_a` | int16 | 100 | A | -32768..32767 | 停机写 |
| 1/22 | `startup_id_ramp_step_a` | uint16 | 10000 | A/TAD | 1..65535 | 停机写 |
| 1/23 | `startup_ramp_tad` | uint16 | 1 | TAD | 1..65535 | 停机写 |
| 1/24 | `current_kp` | int16 | 1000 | — | -32768..32767 | 停机写 |
| 1/25 | `current_ki` | int16 | 1000 | — | -32768..32767 | 停机写 |
| 1/26 | `closed_loop_voltage_limit_factor` | uint16 | 10000 | pu | 1..30000 | 停机写 |
| 1/27 | `current_feedback_sign` | int16 | 1 | — | -1..1 | 停机写 |
| 1/28 | `current_phase_compensation_rad` | int16 | 10000 | rad | -31415..31415 | 停机写 |
| 1/29 | `output_phase_compensation_rad` | int16 | 10000 | rad | -31415..31415 | 停机写 |
| 1/30 | `ac_overcurrent_trip_a` | uint16 | 100 | A | 1..65535 | 停机写 |
| 1/31 | `dc_overvoltage_trip_v` | uint16 | 10 | V | 1..65535 | 停机写 |
| 1/32 | `protection_confirm_samples` | uint16 | 1 | samples | 1..65535 | 停机写 |
| 1/33 | `dab_start_duty_count` | uint16 | 1 | count | 0..2500 | 停机写 |
| 2/0 | `dab_target_duty_count` | uint16 | 1 | count | 0..2500 | 停机写 |
| 2/1 | `dab_ramp_step_count` | uint16 | 1 | count/TAD | 1..2500 | 停机写 |
| 2/2 | `dab_start_count` | uint16 | 1 | count | 0..4999 | 停机写 |

参数直接映射 `g_chb_dab_params`，命令直接映射 `g_chb_dab_command`；不以 reference 原产品的参数含义替代 SST 参数。DAB start duty <= target duty；反馈极性仅+1/-1；open_loop_modulation <= modulation_max。sample_period_s只读，以防串口参数与真实TAD时序不一致。所有普通调参停机写；不会在运行中跨多个控制成员更新参数。

`0/0 run_enable`、`0/1 dab_enable`、`0/3 fault_reset` 和 `F8` 只有 `SST_ALLOW_SERIAL_COMMANDS=(!SST_SIMULATOR_MODE && !SST_ENABLE_DPRAM)` 时生效；fault_reset还要求停机。历史仿真构建使用 `SST_BENCH_INPUT_SIM=1,SST_SIMULATOR_MODE=1,SST_ENABLE_DPRAM=0`，运行控制由仿真器接管，以上串口控制写返回5；当前默认构建通过DPRAM维护点表远控；AnyEyes握手、普通停机参数访问和观测仍可用。有效 `FC` 急停始终把run/dab清零，不受控制源选择阻止。485只使用AnyEyes或VOFA，旧MaintenanceProtocol已移除。

| 功能码范围 | 含义 |
|---|---|
| 3/0..8/29 | 显示索引 n=(group-3)*34+index，n<200有效，只读 |
| 8/30..8/33 | 保留，读0 |
| 19/0 | 参考故障环下一写入槽，1..33 |
| 19/1..19/33 | 参考故障环；启动后首次码前插入0分隔，随后环回；RAM，掉电不保留 |
| 29/0 | 读存储结果；写A55A发起显式参数保存 |
| 29/1 | 读保存是否进行中；写A55A显式恢复 |
| 29/2 | SST映射版本=1 |
| 29/3 | 当前是否允许串口控制 |
| 29/4 | 调试协议1=AnyEyes，2=VOFA |

保存/恢复只允许停机，AnyEyes和DPRAM调用同一存储服务。EEPROM **6/7页**保存v2镜像108字节：50字载荷含37个现代控制参数、ac_mode及4个浮点给定、8个原软件保护/模块巡检参数，另有头和CRC16；不保存run_enable/dab_enable/fault_reset或临时软件保护总使能。兼容读取v1的92字节/42字镜像，旧版本未包含的8项保留当前默认值。保存后台每10ms最多写1字节，先使magic无效，最后提交magic，再逐字节回读验证，不屏蔽全局中断；完成时间随后台节拍和硬件耗时变化。29/0立即应答表示请求接受，继续读取至由4变0；6代表存储失败。恢复停机同步读取对应长度并整体校验后应用，后台通信可能延迟，TAD中断不屏蔽。页4/5旧维护镜像不再作为活动服务加载。默认 `CHB_LOAD_EEPROM_PARAMETERS=0`，显式启用后由runtime停机调用 `SstParameters_LoadPersistent`。实板掉电保存、I2C时序仍须验证。

AnyEyes的组/索引地址保持不变；新增的8项原保护/巡检参数通过DPRAM54点扩展访问，未虚构新的AnyEyes地址。并口点表和直接回读地址见 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)；切换VOFA仍可使用并口启停、参数维护及故障查询。

## 显示/示波器索引

`F2` payload = interval_ms, channel0..channelN-1，N=1..16。每个样本16位、大端；`72`按请求通道顺序返回，不带通道号。`F4`逐个发送 `74` payload=index,value_hi,value_lo，索引0..199循环，10ms一个索引（整表2秒）。示波器以1ms后台时基工作；串口115200/8N1需至少ceil((4+2N)*10000/baud)毫秒，1..3通道最小1ms、4通道最小2ms、16通道最小4ms；实际速率也受后台负载/队列限制，拥塞时跳过采样避免阻塞控制。200项是当前值表，不是触发录波缓冲；跨字段快照不保证来自同一TAD。

| 索引 | 数据 | 比例/显示 |
|---|---|---|
| 0 | CHB_DAB_STATUS.state | 0停止/1预充/2旁路等待/3等PLL/4运行/5故障 |
| 1 | 原CHB fault_code | 原0..6枚举 |
| 2 | 当前故障数量 | uint16 |
| 3..5 | run_enable,dab_enable,ac_mode | uint16 |
| 6..11 | ac_input_closed,precharge_bypassed,ac_pwm_enabled,dab_pwm_enabled,pll_locked,dab_duty_count | uint16 |
| 12,13 | tick_10ms | 32位低字、高字 |
| 14,15 | dc_voltage_average_v,min_v | V×10,uint16 |
| 16 | pll_angle_rad | rad×10000,uint16 |
| 17 | pll_frequency_hz | Hz×100,uint16 |
| 18..21 | id_feedback,iq_feedback,id_reference,startup_id_reference | A×100,int16 |
| 22 | startup_voltage_reference | V×10,uint16 |
| 23 | startup_phase | uint16 |
| 24,25 | startup_tad_count | 32位低字、高字 |
| 26 | voltage_loop_active | uint16 |
| 27 | modulation_actual | ×10000,uint16 |
| 28 | 软件保护开关 | uint16 |
| 30..32 | HA_Port_Volt_A/B/C瞬时量 | V×10,int16 |
| 33..35 | HA_Port_Current_A/B/C瞬时量 | A×10,int16 |
| 40..47 | 8个命令 | 与0组相同比例 |
| 79 | 故障历史最高等级 | 保留参考索引/级别0最重、4无 |
| 80 | 当前故障总数 | 保留参考索引 |
| 81..86 | 6个当前故障码 | 保留参考索引，首项首次观察故障 |
| 87 | 当前最高故障等级 | 0..4 |
| 88 | RAM历史记录数量 | 最多32 |
| 89 | 当前首故障码 | uint16 |
| 100..136 | 37个控制参数 | 与1/2组相同比例 |
| 190 | SST映射版本 | 1 |
| 191 | 保存结果 | 0完成、4忙、6存储失败 |
| 192 | 保存进行中 | bool |
| 193 | 串口控制允许 | bool |
| 194 | 调试协议 | 1/2 |

故障代码与保护来源见 [FAULT_CODES.md](FAULT_CODES.md)。79..86与19组的地址/环语义保留参考实现；新工程故障记录不自动写EEPROM，以免后台诊断触发持久化延迟。

## 命令兼容范围

| 命令 | 行为 |
|---|---|
| FF/7F | 四标签精确匹配或全FF通用连接；当前标签FF 2B 01 00；响应55/AA+6字节版本FF 2B 01 00 01 00，字段统一来自00_Version/SoftVersion.h |
| FA/7A | 连接后查询六段版本；FA无载荷，7A载荷为项目/部件/平台/分支/发布/测试6字节，SST补齐的扩展 |
| FE/7E | 推荐无载荷PC心跳刷新活动时刻，保留此前FE载荷兼容；连接后约每256ms上送既有SST标记FF、实际AC或DAB PWM已使能55/均禁用AA、无故障55/有故障AA；30s无合法指令断链 |
| F0/70、F1/71 | 单功能码读写，原地址格式 |
| F2/72、F3 | 开始/停止波形；停止不新造73帧 |
| F4/74、F5 | 开始/停止状态轮询；停止不新造75帧 |
| F8 | 55启动run+dab、AA停止；保留参考无78应答，受控制源权限 |
| FC | 55 AA CC急停，清run+dab；保留参考无7C应答 |
| F6/F7/F9 | 保留参考状态flag；参考源码未实现76/77/79打包；批量读取由逐项F0实现，故障用原显示槽/19组 |
| FB | 保留8字节55 AA重复校验入口；C6748硬复位尚未绑定，当前不复位、不伪造7B应答 |
| FD | 保留未定义 |

参考入口中的超长帧死循环、未校验通道索引、无效长度/半帧处理已换成有界丢弃及100ms超时；CRC8算法、F0/F1/F2/F4响应字节布局保持原实现。线上任何参数/状态值都不当作裸内存地址。

电子标签不是新的参数地址。本轮集中配置并补齐版本查询，未增加或移动任何功能码、波形或状态索引。参考标签值本来对应F28377S的DCDC产品；当前SST按用户确认保留其匹配值，硬件仍为C6748 + FPGA。精确字节帧、CRC示例、参考命名歧义及CCS串口验证步骤见[ANYEYES_HANDSHAKE.md](ANYEYES_HANDSHAKE.md)。握手成功不改变启停权限。

