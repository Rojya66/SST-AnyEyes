# 当前 V1 工程 DPRAM：代码对应与 CCS 调试步骤

核查日期：2026-11-05。对象：`D:\Code\SST\AnyEyesVersionV1` 当前工作区源码，包含尚未提交的现场调试修改。

本文说明**已经实现的功能**，不把前一份兼容方案当成已实施。本文没有修改编译配置、控制程序或现场参数；下面的写入报文仅供编码核对，不能直接当作现场整定值。

## 1. 先确认当前是否能够接收远控

五项能力均已存在：远控启停、故障复位、PI 参数修改、保护限值修改、故障码回读。但当前源码默认关闭 DPRAM，并由仿真器控制。

配置位置：[ChbBuildConfig.h](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/00_Config/ChbBuildConfig.h:13)。

| 编译宏 | 当前源码值 | 实际 ARM 并口远控所需值 | 意义 |
|---|---:|---:|---|
| `SST_ENABLE_DPRAM` | `0U` | `1U` | 编译进并口访问与维护服务 |
| `SST_SIMULATOR_MODE` | `1U` | `0U` | 为 1 时禁止通信写参数、启停和复位 |
| `SST_BENCH_INPUT_SIM` | `0U` | `0U` | 临时输入注入不能与 DPRAM 远控组合使用 |

`SST_ENABLE_DPRAM=1、SST_SIMULATOR_MODE=1` 可以读状态和参数、得到读请求回复，但写入/遥控返回 `5`。`SST_ALLOW_REMOTE_COMMANDS` 由 `!SST_SIMULATOR_MODE` 派生，见同文件第 120 行。

宏是编译期配置，不能通过 CCS Expressions 在线修改。实际联调时应修改所使用配置的宏，Clean/Rebuild，重新下载对应 `.out`。同时检查 CCS 工程的 Predefined Symbols 是否覆盖头文件的 `#ifndef` 默认值。这里列的是当前源码值，不能据此证明现场已经下载的旧 `.out` 使用何值。

使能 DPRAM 不会自动使能开关量板。`SST_ENABLE_SWITCH_OUTPUT`、`SST_ENABLE_SWITCH_FEEDBACK` 仍需按现场接线配置；它们的关闭会使软件开关状态与实际触点动作存在区别，不能只看软件状态认定接触器已经动作。

## 2. 地址、代码与数据的总路径

所有 `0x6600xxxx` 地址均为 **C6748 DSP 侧字节地址**，16 位访问，相邻字增加 2。ARM 侧应按其硬件映射换算：

```text
DPRAM 字节偏移 = DSP 字节地址 - 0x66000000
DPRAM 16 位字地址 = 字节偏移 / 2
```

例如请求区的字地址是 `0x1600`，不是 `0x2C00`。ARM 的处理器虚拟/物理基地址需由 ARM 驱动确定，不能直接照搬 DSP 的 `0x66002C00`。

| 区域 | DSP 字节首地址 | DPRAM 字地址 | 主机权限/用途 |
|---|---|---|---|
| 旧控制块 1，16 字 | `0x660001E0` | `0x00F0` | W，提交启停、ON/OFF、复位 |
| 扩展维护请求，前 16 字有效 | `0x66002C00` | `0x1600` | W，主机写，DSP 消费 |
| 扩展回复与 54 点回读 | `0x66002D00` | `0x1680` | R，DSP 发布 |
| 扩展状态、量测、当前故障 | `0x66002E00` | `0x1700` | R，DSP 发布 |
| 32 条详细 RAM 故障历史 | `0x66002F00` | `0x1780` | R，DSP 发布 |
| 左邮箱 | `0x66003FFC` | `0x1FFE` | 主机最后写块号，通知 DSP |
| 右邮箱 | `0x66003FFE` | `0x1FFF` | DSP 通知主机，主机读 |

R/W 是协议权限，硬件没有替每个字段提供写保护。不要直接改只读输出区来“改参数”。

扩展区的地址由 [C6748.cmd 第 136 行](/D:/Code/SST/AnyEyesVersionV1/C6748.cmd:136) 定义，数组与段绑定见 [SstDpramExtension.c 第 3 行](/D:/Code/SST/AnyEyesVersionV1/Source/02_Peripheral/01_DPRAM/SstDpramExtension.c:3)，常量见 [SstDpramExtension.h](/D:/Code/SST/AnyEyesVersionV1/Source/02_Peripheral/01_DPRAM/SstDpramExtension.h:6)。

```text
ARM 写数据区，最后写左邮箱（旧控制块=1，扩展维护=21）
  ↓ DPRAM 通知 DSP GPIO101
DPRAMIsr → Flag_RemoteControlRegulate = 1
  ↓ 主循环 TaskSlice_Service
SstDpramHost_ServiceRemoteControl → DPRAM_Read → FLAG_NEW_L
  ├─ 块号 1：SstCommand_ApplyDpram → g_chb_dab_command
  └─ 块号 21：SstDpramMaintenance_Request → request_pending = 1
              ↓ SstDpramMaintenance_Service
             ProcessRequest：帧/CRC/序号校验
              ├─ op=0：读取点表
              ├─ op=1：WritePoint → 参数权限/范围校验 → 实际变量
              ├─ op=2：ApplyCommand → SstCommand_ApplyDpram
              └─ op=3/4：参数保存/加载
              ↓ BuildResponse → 分片发布 → 右邮箱=21

实际启停/复位：ChbDabControl_Service 消费 g_chb_dab_command
实际保护：原 FaultCheck + 闭环 FastStep 的快速保护
故障报告：SstFault_Service → SstDpramMaintenance_Publish → 0x66002E00
```

| 路径节点 | 实际代码位置 | 应观察什么 |
|---|---|---|
| GPIO101 通知 | [Redef_Interrupt.c:594](/D:/Code/SST/AnyEyesVersionV1/Source/03_System/02_ISR/Redef_Interrupt.c:594) | 第 599 行判断 GPIO101，第 603 行置通知标志 |
| 后台调度 | [TaskSlice.c:26](/D:/Code/SST/AnyEyesVersionV1/Source/03_System/01_OS/TaskSlice.c:26) | 控制、故障、并口命令、遥测、扩展服务依次执行 |
| 读左邮箱与旧块 | [DPRAM.C:374](/D:/Code/SST/AnyEyesVersionV1/Source/02_Peripheral/01_DPRAM/DPRAM.C:374) | 第 376 行读邮箱，`FLAG_NEW_L` 应为 1 或 21 |
| 分流旧遥控/扩展请求 | [SstDpramHost.c:34](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/10_Host/SstDpramHost.c:34) | 邮箱 21 调 Request；邮箱 1 调 ApplyDpram |
| 扩展帧校验和操作分发 | [SstDpramMaintenance.c:237](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/10_Host/SstDpramMaintenance.c:237) | `data[16]`、`ack_sequence`、`ack_result` |
| 参数提交 | [SstDpramMaintenance.c:95](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/10_Host/SstDpramMaintenance.c:95) | `point`、`value`，最终成员是否变化 |
| 回复构造 | [SstDpramMaintenance.c:209](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/10_Host/SstDpramMaintenance.c:209) | 回复 3=序号、4=结果、7=实际值 |
| 回复搬运及通知 | [SstDpramMaintenance.c:429](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/10_Host/SstDpramMaintenance.c:429) | 第 439 行分片，第 441 行写右邮箱 |

CRC、参数处理、EEPROM 服务和扩展回复均在后台，未放入 10 kHz 外部中断。GPIO101 ISR 只通知。但如果 10 kHz 中断占满 CPU，后台回复仍会延迟；没有收到 ACK 不能立即认定为协议错误。

## 3. 启机、停机和故障复位具体对应哪里

### 3.1 旧控制块 1：地址不变

每次先准备完整 16 字块，只设置本次需要的命令，其余写 0，最后写左邮箱 `1`。不要保留上一笔 START 再提交 RESET。

| 动作 | DSP 字节地址 | 写入值/位 | 最终命令 |
|---|---|---|---|
| CHB+DAB 启动请求 | `0x660001E4` | `0x0001`，START bit0 | `run_enable=1, dab_enable=1` |
| CHB+DAB 停机请求 | `0x660001E6` | `0x0001`，STOP bit0 | 两路 enable 清 0 |
| 仅 CHB 启动请求 | `0x660001F0` | `0x0002`，ON bit1 | `run_enable=1`；不主动改变已有 DAB 请求 |
| 仅 DAB 启动请求 | `0x660001F0` | `0x0001`，ON bit0 | `dab_enable=1`；不主动改变已有 CHB 请求 |
| 撤销 CHB 请求 | `0x660001F2` | `0x0002`，OFF bit1 | `run_enable=0` |
| 撤销 DAB 请求 | `0x660001F2` | `0x0001`，OFF bit0 | `dab_enable=0` |
| 故障复位请求 | `0x660001F4` | `0x0001`，RESET bit0 | 完全停机后才置 `fault_reset=1` |

代码入口：[SstDpramHost.c:42](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/10_Host/SstDpramHost.c:42) 从控制块提取五个命令字。

命令规则：[SstCommand.c:98](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/04_Control/SstCommand.c:98)：

- 第 105–109 行：STOP 最高优先，立即撤销两路请求并返回；**同一笔 STOP+RESET 不执行复位**。
- 第 112–115 行：OFF 优先于对应 ON。
- 第 118–123 行：提交 START/ON。
- 第 124–125 行：检查完全停机后提交复位。

“完全停机”的准确判据见 [SstCommand.c:13](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/04_Control/SstCommand.c:13)：状态是 STOPPED 或 FAULT，两路命令均为 0，两路 PWM 状态均为 0。FAULT 本身不会导致复位命令被拒绝。

真正的状态机在 [ChbDabControl.c:1477](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/04_Control/ChbDabControl.c:1477)。闭环按 10 ms 节拍推进：`0停机 → 1预充 → 2旁路延时 → 3等PLL → 4运行`，故障为 `5`。命令置 1 不等于立即开脉冲。

真正清故障在 [ChbDabControl.c:1567](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/04_Control/ChbDabControl.c:1567)：第 1578 行消耗复位请求，第 1579 行清控制故障，第 1580 行清端口软件镜像，第 1581 行调用 `FaultLatch_Clear(0U,1U)`，随后进入 STOPPED。底层故障清锁存跨 Timer2 节拍完成，活动故障没有消失时可能再次报故障。

仅 CHB 试验时，应先发 STOP 并确认 DAB 已停，再发 `ON=2`。旧 START bit0 会同时请求 DAB，不能拿它当作“只启动 CHB”。

旧块 1 没有扩展序号 ACK；读回原命令区不能证明动作已经执行。需要观察状态、PWM 和故障回读。

### 3.2 扩展 op=2：相同控制核心，有受理回复

在扩展请求内写 `operation=2`，word6/7/8/9/10 分别是 START/STOP/ON/OFF/RESET。具体位定义与上表相同。处理函数：[SstDpramMaintenance.c:139](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/10_Host/SstDpramMaintenance.c:139)，第 151 行仍调用 `SstCommand_ApplyDpram()`。

首次联调优先用 op=2，便于区分“权限/忙拒绝”与“已受理但状态机尚未运行”。ACK=0 仍只表示受理，不保证预充、PLL 或物理开关已经成功。

## 4. PI 参数、限幅、保护和预充：写到哪个变量

统一规则：请求 `operation=1`，`point=点号`，`value=16位编码`。所有普通参数都要求完全停机且存储空闲。运行中改 PI 返回 `4`，不会在线生效。连 `dc_voltage_ref_v`、`iq_reference_a` 当前也按停机写入处理。

当前现代PI的入口是**邮箱21扩展请求**，不是旧邮箱2。旧块2经 `RemoteRegulate_Dispose_2()` 更新旧参数，不会因其中有PI字段或回显就自动改到 `g_chb_dab_params.voltage_kp/current_kp`。调试应始终追踪最终成员。

**下面的 `0x66002Dxx` 是只读回读地址。写参数应写 `0x66002C08` 的点号和 `0x66002C0A` 的值，并提交完整帧，不能向回读地址直接写。**

### 4.1 给定与 PI

| 点号（十进制） | 实际成员 | 线上编码 | 只读回读地址 | 映射定义行 |
|---:|---|---|---|---:|
| 2 | `g_chb_dab_command.ac_mode` | 0开环/1闭环 | `0x66002D44` | `SstParameterTable.c:22` |
| 6 | `g_chb_dab_command.dc_voltage_ref_v` | 电压×10，无符号 | `0x66002D4C` | `:26` |
| 7 | `g_chb_dab_command.iq_reference_a` | 电流×100，有符号 | `0x66002D4E` | `:27` |
| 24 | `g_chb_dab_params.voltage_kp` | Kp×100000，有符号 | `0x66002D70` | `:45` |
| 25 | `g_chb_dab_params.voltage_ki` | Ki×100，有符号 | `0x66002D72` | `:46` |
| 26 | `g_chb_dab_params.voltage_current_limit_a` | A峰值×100，无符号 | `0x66002D74` | `:47` |
| 33 | `g_chb_dab_params.current_kp` | Kp×1000，有符号 | `0x66002D82` | `:54` |
| 34 | `g_chb_dab_params.current_ki` | Ki×1000，有符号 | `0x66002D84` | `:55` |
| 35 | `g_chb_dab_params.closed_loop_voltage_limit_factor` | pu×10000，无符号 | `0x66002D86` | `:56` |

定义表：[SstParameterTable.c:19](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/01_FuncCode/SstParameterTable.c:19)。有符号项按 int16 二补码传输；例如 `current_kp=-1.249`，raw=-1249，线上为 `0xFB1F`，不是取绝对值。

PI 写入链逐行对应：

1. [SstDpramMaintenance.c:119](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/10_Host/SstDpramMaintenance.c:119)：`n=point-9`，换算成 AnyEyes 共用的参数组/索引。点 24 → n15 → group1/index15；点 33 → n24 → group1/index24。
2. [SstParams.c:174](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/01_FuncCode/SstParams.c:174)：检查停机、只读项及部分交叉约束，第 189 行调表写入。
3. [SstParameterTable.c:100](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/01_FuncCode/SstParameterTable.c:100)：`WriteEntry()` 第 103 行解码符号，第 104 行查 raw 范围，第 109 行执行 `目标float = raw / scale`。**这里是 PI 参数真正进入成员的位置。**
4. 控制器实际使用处：[ChbDabControl.c:311](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/04_Control/ChbDabControl.c:311) 将电压 Kp/Ki 配置到 `s_voltage_pi`；[第 340 行](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/04_Control/ChbDabControl.c:340) 将电流 Kp/Ki 配置到 `s_id_pi/s_iq_pi`。两轴共用同一组电流 Kp/Ki。

例如电压 Kp=0.05 → raw=5000=`0x1388`。电压 Kp 的可编码范围只有 -0.32768～0.32767；电流 Kp/Ki 范围为 -32.768～32.767。通信表的边界不能当作所有控制参数组合都合理的证明。

### 4.2 启动参数与预充门槛

| 点号 | 实际成员 | 编码/单位 | 只读回读地址 | 定义位置 |
|---:|---|---|---|---|
| 11 | `g_chb_dab_params.precharge_voltage_v` | V/模块×10 | `0x66002D56` | 表第32行 |
| 12 | `precharge_stable_10ms` | 10ms计数 | `0x66002D58` | 第33行 |
| 13 | `precharge_timeout_10ms` | 10ms计数 | `0x66002D5A` | 第34行 |
| 14 | `bypass_delay_10ms` | 10ms计数 | `0x66002D5C` | 第35行 |
| 27 | `startup_strategy` | 0/1策略编号 | `0x66002D76` | 第48行 |
| 28 | `startup_udc_offset_v` | V×100，有符号 | `0x66002D78` | 第49行 |
| 29 | `startup_udc_step_v` | V/TAD×1000 | `0x66002D7A` | 第50行 |
| 30 | `startup_id_reference_a` | A×100，有符号 | `0x66002D7C` | 第51行 |
| 31 | `startup_id_ramp_step_a` | A/TAD×10000 | `0x66002D7E` | 第52行 |
| 32 | `startup_ramp_tad` | TAD计数 | `0x66002D80` | 第53行 |

本表省略前缀的成员均属于 `g_chb_dab_params`。预充门槛实际消费位置：[ChbDabControl.c:800](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/04_Control/ChbDabControl.c:800) 的母线就绪判断，以及 [第 1642 行](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/04_Control/ChbDabControl.c:1642) 的预充状态处理。

点 11 与点 52 不是同一个门槛：点 11 决定现代控制状态机是否达到预充条件；点 52 是原逐模块电压巡检的低压门槛。

### 4.3 快速保护、原软件保护、逐模块巡检

| 点号 | 实际变量 | 编码/比较对象 | 只读回读地址 | 赋值/定义 |
|---:|---|---|---|---|
| 39 | `g_chb_dab_params.ac_overcurrent_trip_a` | A×100，瞬时相电流绝对值 | `0x66002D8E` | 表第60行 |
| 40 | `g_chb_dab_params.dc_overvoltage_trip_v` | V×10，有效相模块均压 | `0x66002D90` | 表第61行 |
| 41 | `g_chb_dab_params.protection_confirm_samples` | 连续TAD确认数 | `0x66002D92` | 表第62行 |
| 46 | `OC_level_SET` | 整数A，原电流RMS一级门槛 | `0x66002D9C` | `SstLegacyParameters.c:72` |
| 47 | `OC_time_SET` | 原10ms计数门槛 | `0x66002D9E` | `:73` |
| 48 | `OC_level_SET_L2` | 整数A，原电流RMS二级门槛 | `0x66002DA0` | `:74` |
| 49 | `OC_time_SET_L2` | 原10ms计数门槛 | `0x66002DA2` | `:75` |
| 50 | `OV_level_SET` | 整数V，`SVG_Udc_Ave`滤波量 | `0x66002DA4` | `:76` |
| 51 | `OV_time_SET` | 原10ms计数门槛 | `0x66002DA6` | `:77` |
| 52 | `HA_L_Voltage_Precharge` | V×10，逐模块低压巡检 | `0x66002DA8` | `:78` |
| 53 | `HA_H_Voltage` | V×10，逐模块高压巡检 | `0x66002DAA` | `:79` |

快速保护的判断：[ChbDabControl.c:2157](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/04_Control/ChbDabControl.c:2157)。单相看 A 相物理采样，三相看三相最大绝对电流；过压看有效相均压。达到阈值并连续满足点 41 的确认数后，进入 `ChbDabControl_ForceFault()`。当前该函数在闭环 RUNNING 的 FastStep 分支调用，见 [第 2389 行](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/04_Control/ChbDabControl.c:2389)，不要把它解释成停机时也会按同一方式触发。

原软件保护的判断：[FaultCheck.C:106](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/08_Fault/FaultCheck.C:106)，读取 `PORT_RealTime_DATA.PORT_I_A/B/C` 的 RMS 和 `SVG_Udc_Ave`。计数比较使用 `>`，不是 `>=`；门槛 5 需要计数严格超过 5，实际延时与时基相位有关。

原参数写入：[SstLegacyParameters.c:89](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/01_FuncCode/SstLegacyParameters.c:89)：先检查停机、存储空闲，再校验整组八字镜像，最后 [第 70 行](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/01_FuncCode/SstLegacyParameters.c:70) 赋值。点 52 必须小于点 53，保护电流/过压阈值不能为 0。

逐模块高低压实际判断：[ModuleVoltageCheck.c:88](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/08_Fault/ModuleVoltageCheck.c:88)。点 50、点 40、点 53 的比较对象/确认方式不同，修改其中一项不会同步修改另外两项，也不会自动修改 UNIT/FPGA 内部硬件阈值。

额外核对：点 8 `g_chb_dab_protection_enable` 在当前代码中**允许停机请求写入**，不是只读；实现见 `WritePoint()` 第 117 行转 `SstParams_Write(0,8,...)`，及 [SstParams.c:163](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/01_FuncCode/SstParams.c:163)。本次调试不需要修改该点。点 9 `sample_period_s` 才是明确只读，写入返回 `2`。

## 5. 故障时到底读哪里

### 5.1 控制原因与五位上报码分开读

| 地址 | 类型/含义 | 对应代码 |
|---|---|---|
| `0x66002E06` | uint16，状态0～5 | `telemetry_data[3]=g_chb_dab_status.state` |
| `0x66002E08` | uint16，内部控制故障0～7 | `telemetry_data[4]=g_chb_dab_status.fault_code` |
| `0x66002E10` | uint16，当前故障表首项五位码 | `telemetry_data[8]=SstFault_GetFirstCode()` |
| `0x66002E12` | uint16，最严重等级；0一级、1二级、2三级、3四级、4无故障 | `SstFault_GetLevel()` |
| `0x66002E14` | uint16，当前去重故障数0～32 | `SstFault_GetActiveCount()` |
| `0x66002E40 + 2*i` | uint16，当前故障表i=0～31 | `SstFault_GetCode(i)` |
| `0x66002F06` | uint16，详细RAM历史有效条数 | 历史头word3 |
| `0x66002F20 + 30*i` | 最新第i条历史的五位故障码 | 每条15字，i=0最新 |

| 内部控制码 | 名称 | 对应五位码 |
|---:|---|---:|
| 0 | 无故障 | 0；若另有原保护明细，仍须读故障表 |
| 1 | 预充超时 | 20103 |
| 2 | PLL超时 | 29102 |
| 3 | 快速交流过流 | 29103 |
| 4 | 快速直流过压 | 29104 |
| 5 | 硬件汇总故障 | 按明细为191xx/其他码，无可分类明细时19101 |
| 6 | 配置错误 | 29105 |
| 7 | 开关反馈不符 | 29108 |

故障码定义：[SstFault.h:24](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/08_Fault/SstFault.h:24)。五位码是普通 uint16 数字，例如 29103 十进制，不是 BCD。

完整报告路径：

1. [ChbDabControl.c:1374](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/04_Control/ChbDabControl.c:1374)：锁存内部故障、撤销运行请求、进入 FAULT 并执行故障入口动作。
2. [SstFault.c:190](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/08_Fault/SstFault.c:190)：后台汇总控制故障、`FaultTypeFLAG`、`Port_RealFault` 及原软件 OC/OV；第 217–218 行将控制码3映射成29103。
3. [SstDpramMaintenance.c:478](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/10_Host/SstDpramMaintenance.c:478)：生成状态与故障头；第 503 行复制最多32个五位码。
4. 分片发布到 `DPRAM_SstTelemetry`，ARM 按版本戳读完整快照，再由 ARM 应用通知上位机。DSP 本身没有在这里主动发送 TCP 故障消息。

`0x66002E10` 是本次故障表首项，不是永久的“最早一次硬件跳闸锁存”。旧块3故障位图不保证包含快速软件故障的全部原因，不能只读旧 `0x66000A04`。原 `PORT_FAULT` bit13/bit14 当前含软件 OV/OC 语义，不应直接套用旧 Word 文档的通信/SYNC 含义。

遥测按约30ms节拍构建，再经后台分片发布，见 [Redef_Interrupt.c:124](/D:/Code/SST/AnyEyesVersionV1/Source/03_System/02_ISR/Redef_Interrupt.c:124)。应先记录故障表和历史，再发复位；复位后当前故障表会按条件清零。详细历史为RAM环，重启不保留。

## 6. 扩展请求和回复，按这个顺序操作

### 6.1 请求16字

| word | DSP字节地址 | 字段 | 规定 |
|---:|---|---|---|
| 0 | `0x66002C00` | magic | `0x5353` |
| 1 | `0x66002C02` | version | `1` |
| 2 | `0x66002C04` | sequence | 非零请求序号 |
| 3 | `0x66002C06` | operation | 0读/1写点/2遥控/3保存/4加载 |
| 4 | `0x66002C08` | point | op0/1用十进制0～53 |
| 5 | `0x66002C0A` | value | 参数编码；op3/4必须`A55A` |
| 6 | `0x66002C0C` | start | op2，bit0 |
| 7 | `0x66002C0E` | stop | op2，bit0 |
| 8 | `0x66002C10` | on | op2，bit0 DAB/bit1 CHB |
| 9 | `0x66002C12` | off | op2，bit0 DAB/bit1 CHB |
| 10 | `0x66002C14` | reset | op2，bit0 |
| 11 | `0x66002C16` | reserved | 0 |
| 12 | `0x66002C18` | CRC16 | 前12字、24字节；每字低字节先 |
| 13 | `0x66002C1A` | sequence_copy | 等于word2 |
| 14 | `0x66002C1C` | commit | 最后写`A55A` |
| 15 | `0x66002C1E` | reserved | 0 |

CRC16/IBM：初值FFFF、多项式A001、无最终异或。源码：[SstDpramMaintenance.c:52](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/10_Host/SstDpramMaintenance.c:52)。

逐笔提交：

1. 把 word14 写0，表示正在准备。
2. 写 word0～13 与 word15，未使用命令字均置0。
3. 最后写 word14=`0xA55A`，确认主机侧写操作已完成，再写左邮箱 **21十进制=`0x0015`**。
4. 在读到同序号完整回复前保持请求区不变，不发下一笔。
5. 下一笔换新序号，65535后可以回1；刚得到BUSY的请求在条件恢复后也要换新序号。

最近一笔合法请求若“同序号同内容”会原样回复上次结果而不再执行；“同序号不同内容”返回7。修改点号或数值后，必须重算CRC，同时修改两处序号。

### 6.2 回复怎么判断

先读 `0x66002D00` 版本戳，必须非零偶数；读数据后再读该地址，必须仍为同一偶数，否则整份重读。遥测区、历史区也分别按其 word0 执行此规则。

| 回复地址 | 意义 | 本次请求成功的要求 |
|---|---|---|
| `0x66002D02` / `0x66002D04` | magic / 协议版本 | `5353` / `1` |
| `0x66002D06` | ack_sequence | 等于刚提交的序号 |
| `0x66002D08` | ack_result | `0`表示受理成功 |
| `0x66002D0A` / `0x66002D0C` | 操作/点号回显 | 与请求相符 |
| `0x66002D0E` | 该点实际编码 | 写入后应符合预期，复位命令可能已被消费为0 |
| `0x66002D10` / `0x66002D12` | 存储结果/存储中 | 保存最终要求`0/0` |
| `0x66002D14` | remote_allowed | 远控配置应为1 |
| `0x66002D16` | 点数 | 54 |
| `0x66002D40+2*point` | 各点实际回读 | 与实际成员对应 |

结果码：0成功，1非法点号，2只读，3数值/组合越界，4存储忙或未完全停机，5模式禁止，6存储错误，7格式/CRC/序号冲突。

右邮箱可能很快被旧遥测/插件块号覆盖，应以回复区的版本戳与序号为准，不能只等待“右邮箱当前等于21”。也不要把启动 ACK=0 当作已运行，或把保存 ACK=0 当作已保存完成。

## 7. CCS 具体怎么调试

### 7.1 准备和观察窗口

断点调试安排在无主回路能量、不能实际驱动功率器件的控制板阶段。PWM已开放时不暂停CPU，不在10kHz中断内单步；暂停会改变采样、时基和外部FPGA之间的关系，无法据此判断正常实时流程。

1. 确认本次导入/编译的是 V1 工程，CCS 下载的 `.out` 来自它的实际 Debug/Release 输出目录。
2. 实际 ARM 联调需按第1节配置并重编译下载；不要只改 Expressions 中的运行变量。
3. 打开 `View → Expressions`，添加下面的符号。局部/static符号看不到时，在对应函数栈帧使用 Locals；优化配置可能没有局部变量，此时以命令成员、回复数组和内存地址为准。
4. 打开 `View → Memory Browser`，选DSP核、16位十六进制显示，请求/回复/遥测分别从 `0x66002C00/2D00/2E00` 开始。

```c
Flag_RemoteControlRegulate
FLAG_NEW_L
g_chb_dab_command
g_chb_dab_status
g_chb_dab_params.voltage_kp
g_chb_dab_params.voltage_ki
g_chb_dab_params.current_kp
g_chb_dab_params.current_ki
g_chb_dab_params.voltage_current_limit_a
g_chb_dab_params.precharge_voltage_v
g_chb_dab_params.ac_overcurrent_trip_a
g_chb_dab_params.dc_overvoltage_trip_v
g_chb_dab_params.protection_confirm_samples
OC_level_SET
OC_time_SET
OV_level_SET
OV_time_SET
HA_L_Voltage_Precharge
HA_H_Voltage
Port_RealFault.PORT_FAULT16
FaultTypeFLAG.FaultTypeFLAG16
DPRAM_SstRequest
DPRAM_SstResponse
DPRAM_SstTelemetry
```

不要把左邮箱地址放进自动刷新窗口：DSP侧读左邮箱本身具有应答通知的作用，调试器连续读可能干扰现场握手。看后台已经保存的 `FLAG_NEW_L`，或在读取语句之后观察。

### 7.2 建议断点：逐层确认，不一次全部打开

在源码左边栏双击设置断点；每轮只开当前要定位的一两处。下列行号按本文核查时源码，后续文件变化时以函数名和语句定位。

| 断点 | 文件/行 | 命中说明 | 下一步观察 |
|---|---|---|---|
| B1 | `Redef_Interrupt.c:603` | GPIO101通知进了DSP | 单步该行后标志=1；若真ARM写邮箱不命中，查中断接线/配置 |
| B2 | `SstDpramHost.c:35` | 后台已读邮箱 | `FLAG_NEW_L=21`；旧控制应为1 |
| B3 | `SstDpramMaintenance.c:249` | 已复制16字请求 | Locals的`data[]`逐字与主机发送值一致 |
| B4 | `SstDpramMaintenance.c:269` | 请求通过校验，准备写点 | `data[4]`点号、`data[5]`编码 |
| B5 | `SstParameterTable.c:109` | float参数即将赋值 | `p->address`目标、`p->scale`与`raw`；单步后实际成员变化 |
| B6 | `SstCommand.c:105` | 五字遥控到达统一入口 | `start/stop/pulse_on/pulse_off/reset` |
| B7 | `ChbDabControl.c:1578` | 故障复位已满足条件 | 单步后内部故障清零，随后进入STOPPED |
| B8 | `SstFault.c:218` | 快速OC正映射为29103 | 后台报告链成立，不代表这是首次实际跳闸的时刻 |
| B9 | `SstDpramMaintenance.c:483` | 状态/故障快照正在生成 | `telemetry_data[3/4/8/10]` |

这些都只是定位点；发完请求应 Resume 让后台发布结束，再读完整偶数版本的回复。停在 B3/B5 时主机等不到 ACK 是正常现象。

### 7.3 测试一：先验证“能读、能回复”，不动参数

1. 保持停止，保存当前参数和状态截图/记录。
2. ARM 提交第8节报文A：读点6，序号1。
3. 应依次经过 B1、B2、B3；`FLAG_NEW_L=21`，请求magic=5353。
4. Resume后读回复：版本戳一致且为偶数，ack_seq=1、result=0、point=6。
5. `actual_value` 和 `0x66002D4C` 均应等于当前 `dc_voltage_ref_v×10`（按表编码四舍五入）。**不能预设当前一定为60V。**
6. 记录 `remote_allowed`、点数54、遥测时钟是否持续增加。只读成功说明扩展协议可达，但如果remote_allowed=0，下一阶段写入仍会被拒绝。

### 7.4 测试二：验证PI写入、回读与拒绝条件

1. 先发STOP，确认两路命令/PWM均为0、状态为STOPPED或FAULT、存储空闲。
2. 记录原 `voltage_kp/current_kp` 及线上编码。任选一项，在无功率阶段使用经过确认的测试值提交op=1。
3. 点24电压Kp示例见报文C；在B4应看到point=24、value=5000，B5的raw=5000、scale=100000。
4. 单步赋值后，`g_chb_dab_params.voltage_kp=0.05`。Resume后回复result=0、actual=5000，`0x66002D70=5000`。
5. 如测电流Kp，报文D的 `0xFB1F` 应解释成raw=-1249，成员=-1.249，回读仍为`0xFB1F`。不要在上位机按无符号64287显示这个Kp。
6. 恢复原线上参数，使用新序号与重算CRC；本轮不要发SAVE。
7. 只读边界可用“op=1、point=9”验证应返回2，成员采样周期保持不变。运行中写PI应返回4，可在无功率测试环境验证；不要为测拒绝路径而在有能量状态设置断点。

PI参数写入改变的是活动RAM成员；只有后续闭环控制实际调用调节器才会消费它。通信联调不需要为了观察参数而强行进入RUNNING。

### 7.5 测试三：保护与预充点表逐项核对

按第4节逐项执行“先读原值 → 停机写确认值 → 成员核对 → 读取实际回值 → 恢复原值”。

- 快速OC：point39，60A示例raw=6000，必须对应`ac_overcurrent_trip_a=60.0`，不能误写成60。
- 原一级RMS OC：point46，60A则raw=60，不能套用point39的比例。
- 原OV：point50，80V则raw=80；快速OV point40同样80V则raw=800。
- 现代预充门槛：point11；逐模块低压巡检point52；逐模块高压巡检point53，各自核对不同成员。

这些只是协议/成员对应的验证，不用实际制造过流过压。阈值成功写入不等于已经验证了真实采样、判定延时或FPGA封波。

### 7.6 测试四：启停请求与执行状态分开核对

1. 无功率阶段先发报文G（STOP），确认两路请求清零。
2. 发报文F（仅CHB ON=2）；B6应看到`pulse_on=2`，随后`run_enable=1、dab_enable=0`。
3. Resume后读取状态。闭环无输入/母线/PLL条件时可能停在预充或等待PLL，最终超时故障；这不代表命令未收到，不应为了通信验证屏蔽这些条件。
4. 若验证完整实际启动，撤去全部断点，按现场已确认的电压/采样/保护/开关配置运行，连续轮询`state/startup_phase/PWM/PLL/Udc/Idref`。
5. 再发STOP，新序号，确认状态回STOPPED、两路PWM均关闭。若原先已处于FAULT，STOP只清请求，不会自动清故障。
6. 需要复位时单独发报文H，观察`fault_reset`被消费、控制码=0和状态回STOPPED。保留故障时再次出现的明细，用于判断活动故障是否仍存在。

### 7.7 测试五：故障码与复位链路，不实际制造大电流

可在**主回路无能量、两路PWM确认关闭、ARM不发送启动命令**时做一次仅用于报告链的CCS注入：

1. 停止状态下Halt，确认两路运行请求和PWM状态全为0，`fault_reset=0`。
2. 仅为验证报告链，在Expressions把`g_chb_dab_status.state`写为5、`g_chb_dab_status.fault_code`写为3。不得在有功率运行时这样改状态。
3. Resume。在B8应得到29103；撤掉断点后等待完整快照。
4. 主机应读到：`0x66002E06=5`、`0x66002E08=3`、故障数量至少1、当前表包含29103；单一注入且无其他故障时首项就是29103。
5. 先保存当前表和历史，再发独立RESET请求。预期控制码回0、状态STOPPED，原故障输入清理完成后当前表清零；RAM历史仍可查询。

**这个注入绕过真实采样判定和ForceFault封波动作，只证明“控制原因 → 五位码 → 并口 → ARM读取 → 复位消费”的报告链。它不能证明60A过流判定或硬件封波可靠。** 真实保护链需在另外的受控测试中观察`Chb_CheckSoftwareProtection → ChbDabControl_ForceFault`和实际PWM接口。

### 7.8 暂时没有ARM，如何用CCS验证解析层

仍需编译为DPRAM使能，硬件DPRAM访问正常；若要测写入还需SIMULATOR=0。不能拿`DPRAM=0`版本直接改数组就期望后台处理。

1. 停机、无功率，CCS Halt，在Memory Browser按16位字把报文A写入`0x66002C00`。提交顺序仍是先word14=0，填其余字，最后word14=A55A。
2. 在DSP视角地址`0x66003FFC`写`0x0015`；再在Expressions把`Flag_RemoteControlRegulate`置1，Resume。
3. 在B2核实`FLAG_NEW_L=21`，B3核实data[]与请求一致，随后读完整回复。
4. 某些双口RAM邮箱寄存器具有端口方向或只读限制，DSP/仿真器写左邮箱未必能把它变成所需值，也不一定给DSP本身产生GPIO中断。若B2仍不是21，不要把测试当成成功。
5. 该硬件不支持上述写法时，可仅在无功率测试中Halt于B2（`DPRAM_Read()`执行后），把已保存的`FLAG_NEW_L`改为21再Resume；使后台进入扩展请求分支。若首次未能进入B2，可先置通知标志1。这只替代了邮箱分流输入。

这两种CCS办法都跳过了部分ARM硬件握手。最终必须撤掉人工注入、由真实ARM写左邮箱，重新核对B1→B2→B3以及回复；只有这一步才能覆盖并口端口方向、GPIO101和ARM驱动。

## 8. 可核对的完整报文（16个16位十六进制字）

每行从word0到word15，均从`0x66002C00`起写。提交后写左邮箱`0015`。报文序号只是示例，应按实际会话分配新序号；其中修改参数的值不是对当前台架的整定建议。

```text
A 读点6(UdcRef)，seq=1：
5353 0001 0001 0000 0006 0000 0000 0000 0000 0000 0000 0000 2FA2 0001 A55A 0000

B 写点6=60.0V，seq=2：
5353 0001 0002 0001 0006 0258 0000 0000 0000 0000 0000 0000 3AE9 0002 A55A 0000

C 写点24(电压Kp)=0.05，seq=3：
5353 0001 0003 0001 0018 1388 0000 0000 0000 0000 0000 0000 42B0 0003 A55A 0000

D 写点33(电流Kp)=-1.249，seq=4：
5353 0001 0004 0001 0021 FB1F 0000 0000 0000 0000 0000 0000 C920 0004 A55A 0000

E 写点39(快速OC)=60.0A，seq=5：
5353 0001 0005 0001 0027 1770 0000 0000 0000 0000 0000 0000 789E 0005 A55A 0000

F 仅CHB启动请求(ON bit1)，seq=6：
5353 0001 0006 0002 0000 0000 0000 0000 0002 0000 0000 0000 5BB4 0006 A55A 0000

G 两路STOP，seq=7：
5353 0001 0007 0002 0000 0000 0000 0001 0000 0000 0000 0000 9659 0007 A55A 0000

H 独立RESET，seq=8：
5353 0001 0008 0002 0000 0000 0000 0000 0000 0000 0001 0000 515D 0008 A55A 0000

I SAVE，seq=9（仅需持久化时使用）：
5353 0001 0009 0003 0000 A55A 0000 0000 0000 0000 0000 0000 7EE6 0009 A55A 0000

J 读点34(电流Ki)，seq=10：
5353 0001 000A 0000 0022 0000 0000 0000 0000 0000 0000 0000 4E1C 000A A55A 0000
```

注意十六进制混淆：点24=`0018`；点33=`0021`；点39=`0027`。邮箱21十进制=`0015`，不要写成`0021`。

主机生成CRC及完整请求可采用以下等价Python逻辑，负参数先按表乘比例，再编码为16位：

```python
def make_request(seq, op, point=0, value=0, start=0, stop=0,
                 on=0, off=0, reset=0):
    if not 1 <= seq <= 65535:
        raise ValueError("sequence must be 1..65535")
    words = [0x5353, 1, seq, op, point, value & 0xFFFF,
             start, stop, on, off, reset, 0, 0, seq, 0xA55A, 0]
    crc = 0xFFFF
    for word in words[:12]:
        for byte in (word & 0xFF, (word >> 8) & 0xFF):
            crc ^= byte
            for _ in range(8):
                crc = (crc >> 1) ^ (0xA001 if crc & 1 else 0)
    words[12] = crc
    return words

print(" ".join(f"{w:04X}" for w in make_request(3, 1, 24, 5000)))
```

## 9. 需要长期保留参数时，额外验证保存

普通写入只改RAM。需要保存时，完全停机后发op3、value=A55A；实现见 [SstParameterStore.c:162](/D:/Code/SST/AnyEyesVersionV1/Source/04_App/01_FuncCode/SstParameterStore.c:162)。

- ACK=0仅表示保存任务建立。
- 持续轮询回复的`storage_pending`，等待0，并确认`storage_result=0`才算保存完成；6是存储校验/访问错误。
- op4、value=A55A是主动加载，需要停机且存储空闲。
- 当前`CHB_LOAD_EEPROM_PARAMETERS=0`，因此即使SAVE成功，重启也不代表自动采用EEPROM值。需明确选择启用启动加载后重编译，或者每次停机后主动LOAD，再核对实际回值。
- 保护总使能、运行请求、复位请求不持久化；不要试图保存“启动状态”。

## 10. 对不上时按现象查

| 现象 | 首先检查 |
|---|---|
| 新遥测区一直没有有效magic/偶数版本 | 是否下载DPRAM使能版本；链接地址；后台是否运行；遥测节拍是否推进 |
| ARM提交后B1不命中 | ARM写的是正确端口/字地址吗；是否最后写邮箱0015；GPIO101/中断配置 |
| B1命中但B2不命中 | 后台是否被10kHz ISR挤占；是否停在其他断点；当前加载文件是否对应源码 |
| B2命中但块号错误 | ARM偏移、16/32位步长、大小端、是否有人抢读左邮箱或覆盖邮箱 |
| 回复7 | data[]与原报文逐字比对；CRC字节顺序；两个序号；A55A最后提交；是否复用序号改内容 |
| 回复5 | SIMULATOR_MODE仍为1，查看remote_allowed；确认编译宏与下载文件 |
| 回复4 | 状态/两路命令/两路PWM是否全部满足停机；是否仍在保存；条件恢复后换新序号 |
| 回复0但电压Kp差1000倍 | 是否用了point24的×100000；不要套电流Kp的×1000 |
| ACK0但没有脉冲 | 只是命令受理；查看状态、预充、PLL、实际故障与PWM；START与仅CHB ON的区别 |
| STOP+RESET后故障仍在 | 同帧STOP直接return，分两笔：先STOP确认，再RESET |
| 复位ACK0但马上又报故障 | 查看活动硬件/软件故障是否仍存在、底层锁存清理与开关反馈；先保存故障明细 |
| 回读参数“成功”，实际变量没变 | 是否直接写了只读输出区；是否仅看旧Block2回显；应按扩展请求和实际成员核对 |
| 右邮箱没看到21 | 可能被旧遥测覆盖，直接轮询稳定版本和ack_sequence |

完整54点的缩放、范围、权限仍见 [DPRAM_PARAMETERS.csv](/D:/Code/SST/AnyEyesVersionV1/docs/DPRAM_PARAMETERS.csv)，逐字总表见 [DPRAM_ADDRESSES.csv](/D:/Code/SST/AnyEyesVersionV1/docs/DPRAM_ADDRESSES.csv)。旧文档中的默认编译配置与示例值不能替代本次源码和现场参数回读。

