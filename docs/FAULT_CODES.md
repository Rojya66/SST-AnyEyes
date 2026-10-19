# SST 故障编码与后台报告

`SstFault.c/.h` 是后台故障报告层。`SstFault_Init()` 在控制运行变量初始化后调用，`SstFault_Service()` 在主循环调用。它只读取原有 RAM 状态，不写 FPGA、不关闭中断、不操作 PWM、不调用 EEPROM，不在 10 kHz 控制中断中增加函数或循环。即时封波继续由原 `FaultCheck.C`、`ChbDabControl_ForceFault()` 和 FPGA/UNIT 硬件完成。

## 与参考框架的关系

编码遵循 `F28377S_Plat/Source/04_App/08_Fault/ErrorMacro.h` 的五位十进制 `ABCDE`：`A` 表示层（1 硬件、2 算法、3 应用），`BC` 表示模块，`DE` 表示条件。模块号 `91` 是本工程的 SST 扩展，不改变参考工程已有编码。预充超时直接沿用参考编码 `20103`。

等级数值与参考 `E_FAULT_LEVEL` 一致，不能误按 1～5 发送：

| 宏 | 线上数值 | 含义 |
|---|---:|---|
| `SST_FAULT_LEVEL_1` | 0 | 一级，硬件/模块保护报告 |
| `SST_FAULT_LEVEL_2` | 1 | 二级，控制算法及配置故障 |
| `SST_FAULT_LEVEL_3` | 2 | 三级，应用/兼容状态报告 |
| `SST_FAULT_LEVEL_4` | 3 | 四级，告警；本次无新增告警检测器 |
| `SST_FAULT_LEVEL_NONE` | 4 | 无故障 |

当前最高等级取数值最小者。代码 `0` 表示无故障。等级只服务于报告，不能据此重新执行参考平台的 `FORCE_SHUT_PWM()`；源 SST 的即时故障动作保持原样。

## 当前实际控制与保护路径

| 编码 | 级别 | 真实输入/保护来源 | 可辨识程度 |
|---:|---|---|---|
| 20103 | 二级 | `CHB_FAULT_PRECHARGE_TIMEOUT` | 闭环预充超时 |
| 29102 | 二级 | `CHB_FAULT_PLL_TIMEOUT` | 闭环 PLL 等待超时；单相、三相均包括运行中连续200TAD（20 ms）失锁/失压 |
| 29103 | 二级 | `CHB_FAULT_AC_OVERCURRENT` 或 `HVAC_OC_Fault` | 快速 AC 过流/旧 HVAC 软件过流；旧一级、二级过流共用一标志，不能事后区分 |
| 29104 | 二级 | `CHB_FAULT_DC_OVERVOLTAGE` 或 `HVAC_OV_Fault` | 快速直流过压/旧 HVAC 软件过压；原始 `control_fault` 随历史保存以区分来源 |
| 29105 | 二级 | `CHB_FAULT_BAD_CONFIGURATION` | 模块数量/参考量等配置检查失败；单相、三相均包括启动/运行中切模式、非法数值/采样及有效相母线低于预充门槛 |
| 19101 | 一级 | `CHB_FAULT_HARDWARE` 或端口总故障，但无可辨识分类 | 未分类汇总；不能解释为某个具体器件损坏 |
| 19102 | 一级 | `FaultTypeFLAG.Port_OverCurrent` 或端口 bit10 | 模拟插件/端口硬件过流；具体通道仍需原寄存器证据 |
| 19103 | 一级 | `FaultTypeFLAG.Port_OverVolt` 或端口 bit12 | 端口硬件过压兼容报告；当前 HVAC 的 AD 保护分支主要归类为过流 |
| 19104 | 一级 | `FaultTypeFLAG.Module_Fault` 或端口 bit6，尚无模块细分 | UNIT/模块故障汇总 |
| 19105 | 一级 | `FaultTypeFLAG.Module_OverCurrent` | 模块过流，来自原 FO/模块锁存解析 |
| 19106 | 一级 | `FaultTypeFLAG.Module_OverVolt` | 模块过压，来自原 FO/模块锁存解析 |
| 19107 | 一级 | `FaultTypeFLAG.Module_OverTemp` | 模块过温，来自原 FO/模块锁存解析 |
| 19108 | 一级 | `FaultTypeFLAG.Module_50Hz_H` 或端口 bit7 | CHB/50 Hz 桥故障，不能据此定位具体 IGBT |
| 19109 | 一级 | `FaultTypeFLAG.Module_DAB_H` | DAB 桥硬件故障，不能据此定位具体 IGBT |

上述细分只依赖原程序已产生的分类。如果原程序仅锁存 `CHB_FAULT_HARDWARE`，新模块不会额外遍历 FPGA/UNIT 来猜测具体原因。后台读取的 FO_A/B/C 位图、FB、PortLatch、模块处置计数作为历史证据保留；因原复位不一定清这些 RAM 影子变量，它们不单独触发新故障。

快速软件 OC/OV 原有 `g_chb_dab_protection_enable` 开关和开环绕过条件保持不变。原 `OC_OV_LV_faultcheck()` 的运行状态条件、计时和门槛保持不变。FPGA FAULT/FO/FB 独立硬件故障不受上述软件开关屏蔽。新报告层不重新比较电流/电压，也不把关闭的软件保护变相恢复。

### 两套软件保护的阈值与计时

下表列出高压参数组（`SST_LOW_VOLTAGE_100V_TEST=0`）。当前默认 100 Vac 单相参数将原一级/二级过流改为 3/4 A、原过压改为 42 V，快速过流/过压改为 3 A/42 V；确认时间沿用原设置。42 V 检查相模块均压或其合成滤波值，均不是逐模块最大电压保护。低压参数及恢复三相的方法见 [PI_REFACTOR_AND_LOW_VOLTAGE.md](PI_REFACTOR_AND_LOW_VOLTAGE.md)。

| 保护来源 | 默认阈值及比较对象 | 确认规则 | 设置位置 |
|---|---|---|---|
| 原HVAC一级过流 | `OC_level_SET=70 A`；三相电流200点有效值，任一相严格大于阈值 | `counter_OC_time > OC_time_SET`，默认设置100 | `ProtectionConfig.h`默认宏 → `ProtectionData.c`运行变量 |
| 原HVAC二级过流 | `OC_level_SET_L2=75 A`；同一有效值口径 | `counter_OC_time1 > OC_time_SET_L2`，默认设置5 | 同上 |
| 原HVAC过压 | `OV_level_SET=830 V`；三相模块均压合成并滤波后的`SVG_Udc_Ave` | `counter_OV_time > OV_time_SET`，默认设置5 | 同上 |
| 快速过流 | `ac_overcurrent_trip_a=120 A`；每TAD标定相电流绝对值最大值，单相仅取A相 | 达到阈值后连续`protection_confirm_samples=5`个TAD确认，未达到时清计数 | `ChbDabControl.c`中`g_chb_dab_params`初始化表 |
| 快速过压 | `dc_overvoltage_trip_v=780 V`；三相各自模块均压的最大值，单相仅取A相模块均压 | 同上 | 同上 |

原保护的`counter_*_flag`是计时使能标志，`counter_*_time`在Timer2中断中每10 ms递增；原`*_time_SET`单位是Timer2节拍，不是TAD。严格大于判据使默认100/5需101/6次递增；持续超限且运行检查未中断时，从置位计时标志到确认约1.00～1.01 s / 50～60 ms，另受节拍相位、TAD检查和有效值发布时间影响。一级过流与过压进入95%～100%回差带后仍保留计时标志和累计计数，低于95%才清；二级过流在三相均不超过75 A时清计数，没有95%回差。

电流有效值累积200个有效TAD样本后等待后台发布；10 kHz时积累窗口为20 ms，发布时间还取决于后台服务。快速保护直接使用每TAD的标定电流样本，不能把120 A与原70/75 A按同一测量口径比较。5个TAD在10 kHz下是名义0.5 ms窗口，实际保护响应仍需板级测量。

CCS可修改上述运行变量；修改默认值需改相应宏或初始化表。AnyEyes的组/索引`1/30`、`1/31`、`1/32`分别映射快速过流门槛、快速过压门槛、快速确认样本数，停机可写；原保护阈值不与这三个参数自动同步。DPRAM扩展点39/40/41复用这三个快速参数，点46..51维护原OC/OV阈值和10ms计时，点52/53分别为 `HA_L_Voltage_Precharge` 和 `HA_H_Voltage`（0.1V/count）。所有阈值维护要求完全停机、存储空闲，并校验范围；精确回读地址见 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)。

`Udc_check()` 的逐模块低压/高压扫描仍保留原功能；其中欠压是预充状态信息，高压分支中的直接封波代码在源工程已注释。报告层不把这些信息变成新增欠压或过压停机。`dc_voltage_min_v` 在三相时是三相均压中的最小值，单相时等于A相均压，并非所有模块电压的最小值。

单相、三相闭环的快速 OC/OV 检查均在 PI 计算及参考写入之前执行，确认故障的当拍不再更新调制。运行失锁、数值/模式/逐相母线检查是公共控制运行约束，独立于快速 OC/OV 总使能，不会因关闭该软件保护开关而绕过；它们复用既有故障编号及上报地址。早期单相审查保留在 [PHASE_A_CLOSED_LOOP_AUDIT.md](PHASE_A_CLOSED_LOOP_AUDIT.md)，公共路径修正见 [PI_REFACTOR_AND_LOW_VOLTAGE.md](PI_REFACTOR_AND_LOW_VOLTAGE.md)。

## 兼容状态码，不新增检测器

下面编码可以报告原兼容位一旦被置位的状态；它们不代表本次为 CHB 简化状态机新增了相应检查。部分标志来自其他端口/旧流程，在当前 HVAC 简化流程中没有主动写入路径，应在上位机标为“兼容状态”。

| 编码 | 级别 | 输入 |
|---:|---|---|
| 19110 | 一级 | `FaultTypeFLAG.Module_Single`，旧模块单体故障 |
| 19111 | 一级 | `FaultTypeFLAG.Module_LowerVolt` 或端口 bit11，旧模块欠压 |
| 19112 | 一级 | `FaultTypeFLAG.Module_OverVoltLevel2`，旧模块二级过压 |
| 19113 | 一级 | 端口 bit2，风机状态 |
| 19114 | 一级 | `FaultTypeFLAG.Controller` 或端口 bit3，控制器状态 |
| 19115 | 一级 | 端口 bit4，辅助供电状态 |
| 19116 | 一级 | 端口 bit5，急停状态 |
| 29106 | 二级 | 端口 bit8，相序兼容状态 |
| 29107 | 二级 | 端口 bit9，频率兼容状态 |
| 39101 | 三级 | `FaultTypeFLAG.Contactor` 或端口 bit0，接触器兼容状态 |
| 39102 | 三级 | `FaultTypeFLAG.Breaker` 或端口 bit1，断路器兼容状态 |
| 39103 | 三级 | **仅** `FaultTypeFLAG.Communication` |
| 39104 | 三级 | **仅** `FaultTypeFLAG.Sync` |

`main.H` 对端口 bit13/14 的旧注释是“通信/SYNC”，但当前 HVAC 软件故障处置实际用它们记录 OV/OC。报告层不按这两个端口位产生 39103/39104，而使用已锁存的 `HVAC_OV_Fault/HVAC_OC_Fault`。若只有总故障和无歧义细分以外的位，保留 19101 与原始故障字，不猜测来源。

## 首故障、当前故障和历史

- 当前表容量32，按后台首次观察顺序保存，同一故障周期内相同编码只记录一次。`GetCode(0)` 是本次首个**后台已观察**代码；多源在同一后台调用前发生时无法恢复亚周期的精确先后。优先保留 `g_chb_dab_status.fault_code` 明确的软件原因，再记录旧软件和硬件细分。
- 模块细分可能在后续旧中断步骤中才形成。先记录的汇总代码保留，随后追加细分类别；这表示同一故障周期的诊断补充，不是重复事件。
- 原控制状态回到 `STOPPED`、`CHB_FAULT_NONE`、启停请求均清零、端口/类型/旧软件故障已清除后，当前表清空，历史保留，同一故障可再次入历史。`FaultLatch_Clear` 两步复位期间不清表、不重复建档。
- 历史为32条 RAM 环形记录，最新记录索引0，覆盖最老记录。记录包含编码、等级、10 ms后台观察时刻、递增序号、源控制故障和原 Port/Type/FO/FB证据。它不是中断触发瞬间的同步故障录波；原 FT3/录波路径继续保留。
- 历史掉电丢失。本次不占用既有 EEPROM 参数地址，不声称实现参考平台的大容量 EEPROM 快照保存。历史最高等级只计算目前结构化32条环中保留的记录。
- 所有读 API 与 Service 在同一后台上下文使用；C/C++ 链接均可用。读源状态时不全局屏蔽中断；若控制状态/故障码在采样前后变化，本次放弃，下一轮再读。多个原 RAM 标志不是硬件原子快照，短于一次后台调度且未锁存的瞬态可能无法记录。

## AnyEyes 地址与报文边界

故障管理器只给通信层 C API，不自行改写任何协议。参考框架保留 `0xF9` 故障查询命令和 `0x79` 应答编号，但原实现只置查询flag、没有实际79帧打包；本移植也不新增该应答。故障通过16位码、显示槽79（历史最高等级）、80（当前数量）、81～86（六个当前故障）及组19历史故障地址读取，由 PcDebug/功能码适配层统一处理。该层应读取上述接口，不能直接读取后台私有数组，也不能因故障查询触发额外封波或 EEPROM写入。实际地址和帧字段以 [ANYEYES_MAPPING.md](ANYEYES_MAPPING.md) 与 [COMMUNICATION.md](COMMUNICATION.md) 为准。

`SstFault_GetLegacyHistoryWord(index)` 专供组19：共34字，索引0是下一写入位置（1～33），索引1～33为按物理环位置存放的故障码。首次初始化后第一个故障前先写一个0作为会话分隔，推进到下一个槽，再写故障码；每次写完位置加一，到34回到1。这保持参考 `FaultDeal.cpp::SaveFaultCode2Mem()` 的地址/环格式，不能把组19误当作“最高等级、数量、最新优先列表”。独立结构化历史API仍是32条、最新索引0。两种视图均为RAM，不模拟EEPROM持久化。

源 SST 的 EMIFA/FPGA 故障通知地址 `0x6000082A/2C/2E`、DPRAM布局和故障字保持原样。报告码是新上位机表示层，不替换原16位端口故障字。

## DPRAM现代故障快照

原公共块 `0x66000A04` 继续提供旧16位端口位图。现代遥测区 `0x66002E00` 和历史区 `0x66002F00` 另提供五位故障码、等级、首故障、当前数量、源控制故障、原始位图及32条历史。现有五位十进制故障码使用一个16位无符号数字编码，不是ASCII或BCD，也不能从旧16位故障字直接推导；32位双字只用于节拍、计数和事件序号。读取前后应核对相同偶数版本戳，以避免分片发布中的混合数据；字段地址和位定义见 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)。故障历史仍是RAM，掉电丢失，保存参数不保存故障环。

## 验证

执行 `python tests/fault_reporting.py`。测试直接编译真实 `SstFault.c` 函数体，只用普通变量替代外部硬件状态，覆盖六类 CHB 故障映射、首故障、当前/历史等级、重复来源去重、延后细分、两步复位、复位后再次触发、残留锁存不误报、32条历史回绕和接口边界。主机测试不证明板上中断延迟或故障硬件响应时间；这些仍需原保护链板级验证。
