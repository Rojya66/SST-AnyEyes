# AnyEyesVersionV1 与 ARM DSP DPRAM 文档兼容性核查

核查日期：2026-11-05。核查对象：`D:\Code\SST\AnyEyesVersionV1` 当前工作目录源码，包括已有未提交修改。对照文件：用户提供的《ARM DSP DPRAM 与 LAN 通讯协议及上位机界面设计》，共核对 19 张表及正文。

结论：**基础 DPRAM 地址和块布局兼容，控制、参数、状态和故障语义仅部分兼容，不能按此 Word 文档直接完成全部上位机功能。** 文档混用了旧端口控制、已移除的 A5 5A 串口维护协议和新 CHB 状态机描述；没有覆盖当前并口扩展协议。

本次只新增核查记录，没有修改程序、编译配置或原 Word，没有进行 ARM/DPRAM 硬件联调。以下结论针对源码能力，不证明现场烧录的 out 文件与该源码一致。

## 1 当前默认配置没有进入 DPRAM 远控模式

`Source/04_App/00_Config/ChbBuildConfig.h` 当前默认：

```c
#define SST_ENABLE_DPRAM 0U
#define SST_SIMULATOR_MODE 1U
```

要由 ARM 经并口远控，应选择 `SST_ENABLE_DPRAM=1`、`SST_SIMULATOR_MODE=0` 后重新编译下载。若二者均为 1，则会提供并口读取、遥测和维护回复，但拒绝并口参数修改及运行命令。当前 Debug 工程配置中未发现覆盖这两个宏的定义。

UART 协议由 `SST_DEBUG_PROTOCOL` 单独选择 AnyEyes 或 VOFA，不承担本文并口维护协议。若启用了临时输入注入 `SST_BENCH_INPUT_SIM`，须先关闭；该测试配置与 DPRAM 互斥。

开关量硬件输出及反馈还由 `SST_ENABLE_SWITCH_OUTPUT`、`SST_ENABLE_SWITCH_FEEDBACK` 分别控制，目前默认均为 0。进入 DPRAM 模式本身不会自动开启接触器硬件输出或反馈联锁。状态机中的开关请求不应直接当作现场触点反馈。

## 2 地址和传输布局

下列访问方向从 ARM 角度说明，地址为 DSP 字节地址。逻辑字地址 = `(DSP地址 - 0x66000000) / 2`；ARM 使用自身 FSMC 基址加同一字节偏移，不能直接照抄 DSP 地址。

| 区域 | DSP 地址 | 字数 | 主机方向 | 与 Word 对照 |
|---|---|---:|---|---|
| 控制 Block1 | 0x660001E0～0x660001FE | 16 | 写 | 地址及字段排列一致，功能有删减 |
| 遥调 Block2 | 0x660003FE～0x66000452 | 43 | 写及读回 | 地址及字段排列一致，很多字段不生效 |
| 公共遥测 Block3 | 0x66000A00～0x66000A40 | 33 | 读 | 排列一致，部分字段未更新或含义不同 |
| 插件 U2～U17 | 首址 0x66000D00，每插件间隔 0x200 字节 | 每块128 | 读 | 地址一致，部分插件不刷新 |
| 左邮箱 | 0x66003FFC | 1 | 写通知 | 1遥控、2遥调；新增21维护 |
| 右邮箱 | 0x66003FFE | 1 | 读通知 | 3遥测、4～19插件、20旧遥调回读；新增21维护回复 |

证据：`C6748.cmd:94` 起、`DpramConfig.h`、`DpramRegister.h`、`DPRAM.C:374`。

GPIO101 下降沿 ISR 只置位后台读取标志，与 Word 描述一致。控制块应清零后只填写本次动作，完整写块后最后写邮箱；不要把写入值的回读当执行成功。

右邮箱不是事件队列：`SstDpramHost_ServiceTelemetry()` 每轮先发布块3，再发布一个轮询块，后者可以马上覆盖邮箱3；扩展回复21也共用此邮箱。只等待邮箱恰好等于3或21的轮询方案可能漏掉通知。ARM 应定时读取公共块，扩展回复按序号和版本戳确认，不能依赖捕获每次邮箱值。物理中断清除行为仍需结合 ARM 驱动和 DPRAM 器件核验，不能自行增加反向写邮箱 ACK。

## 3 控制块兼容范围

真正执行入口是 `SstDpramHost_ServiceRemoteControl()` → `SstCommand_ApplyDpram()`，不是文件中保留的旧 `RemoteControl_Dispose()`。

| 字段 | 地址 | 当前实际行为 |
|---|---|---|
| COM_START_PORT bit0 | 0x660001E4 | 同时请求 CHB 与 DAB；闭环实际发波由状态机和联锁决定 |
| COM_STOP_PORT bit0 | 0x660001E6 | 撤销 CHB 与 DAB 请求；优先于同帧启动 |
| COM_PLUSE_ON bit0/bit1 | 0x660001F0 | 分别提交 DAB/CHB 使能请求，不直接解锁硬件 |
| COM_PLUSE_OFF bit0/bit1 | 0x660001F2 | 分别撤销 DAB/CHB 请求；同位 OFF 优先 |
| COM_FAULT_CLEAR bit0 | 0x660001F4 | 完全停机且存储不忙时请求复位；不等于硬件故障源已经消失 |
| COM_PORT_MODE | 0x660001E8 | 未传入当前有效遥控入口；不切换现代开闭环模式 |
| COM_SWITCH_ON/OFF | 0x660001EC/0x660001EE | 未传入当前入口；不执行文档中的独立合分闸 |
| PORT_SWITCH_EN | 0x660001F6 | 未传入当前入口 |
| PORT_NAME/SENSORTYPE/GRIDTYPE | 0x660001F8/0x660001FA/0x660001FC | 未传入当前入口；不能靠写这些字配置端口、PT/CT或电网属性 |
| START/STOP bit1～3 | 同启动/停止字 | 本工程不据此控制 HVDC/LVDC/LVAC |

证据：`SstDpramHost.c:39`、`SstCommand.c:98`、`ChbDabControl.c:1347`。

Word 第4节的正常停止含“降功率”描述，当前核心是撤销请求后封波和切换停机状态，没有独立的功率渐降停机阶段。Word 的预充350V也是旧示例，当前低压分支 `params.precharge_voltage_v` 默认25V；应读取参数实际值，不能写死界面阈值。旧模块预充筛查阈值是另一个参数。

## 4 旧遥调块不等于现代参数接口

块2只在允许远控、完全停机且参数存储不忙时交给 HVAC 的 `RemoteRegulate_Dispose_2(0)`。

- `PORT_V_SET`：更新旧 `PORT_Regulate_DATA`、`HA_Module_RefUdc`、`Udc_Ref` 等旧变量，并写 FPGA 0x6000062A。低压分支当前允许20～250V，越界回退100V。**不写现代 `g_chb_dab_command.dc_voltage_ref_v`。**
- `STATCOM_Q_SET`：旧缓存按 raw×100 更新，FPGA 0x6000062C 写 raw/200（整数除法）。**不写现代 `iq_reference_a`。**
- 系统模式、AD/FO/FB使能、P/Q/F、谐波、下垂、模块配置和旧 PI 等处理段均被注释，缓存里出现新值不能证明控制参数已应用。
- 旧 PI 字段也不是现代 `voltage_kp/ki`、`current_kp/ki` 的直接映射。
- 收到右邮箱20，只说明旧块2缓存完成回写。它不代表全部43字已应用，更不代表 EEPROM 保存成功。
- 当前旧块2入口不自动写 EEPROM；现代参数保存使用扩展邮箱21的显式 SAVE。

证据：`SstDpramHost.c:48`、`DPRAM.C:1484`、`:1543`、`:1608`、`:1786`、`:1924`、`:335`。不能因旧文件里还有其他端口或旧函数的 EEPROM 调用，就认定当前 HVAC 有效入口会保存。

## 5 遥测、状态和故障的具体差异

### 5.1 工程量

| 地址/字段 | 当前有效含义及缩放 |
|---|---|
| 0x66000A06 PORT_P | signed16，raw/10 kW |
| 0x66000A08 PORT_Q | signed16，raw/10 kvar |
| 0x66000A0A PORT_V_A | 当前单相宏下为 AN 电压有效值，1V/count；三相宏下恢复线电压遥测 |
| 0x66000A10 PORT_I_A | A相电流有效值，raw/10 A |
| B/C V/I | 当前单相试验不作为有效三相测量；相关通道按单相路径置零 |
| 0x66000A1E I_A_2 | `HA_Grid_Current_A` 的200点RMS，raw/10 A；不能仅凭字段名解释成变压器二次侧电流 |
| 0x66000A24 DC_V | 当前 HVAC 遥测函数没有更新该字，不能用它显示有效的模块/母线直流电压 |
| V_A_2/B_2/C_2、PORT_I_N | 当前 HVAC 遥测函数未赋值，不能按文档中的名称假定存在有效数据 |

模块均压应读取扩展 `0x66002E20`，raw/10 V；各模块直流电压可读取实际刷新的光通信插件块。扩展均压是模块平均值，不是六模块串联总电压。

证据：`DPRAM.C:3038`、`MeasurementService.c:33`、`:76`、`ADC_Sample.C:198`、`SstDpramMaintenance.c:491` 附近。Block3其余谐波字段虽有打包语句，有效性仍取决于对应测量生产者，不能把有地址当作有有效量测。

### 5.2 PORT_RUN_STATUS 与 PORT_STATUS

`0x66000A00` 是复合主状态字，应先取 `raw & 0x003F` 解码运行状态，不能把完整16位字直接当现代状态枚举。

| 现代状态 | Block3 P_Status | 扩展0x66002E06 |
|---|---:|---:|
| 停机/待机 | 0x06 | 0 |
| 预充 | 0x10 | 1 |
| 旁路等待 | 0x14 | 2 |
| 等待PLL | 0x16 | 3 |
| 运行 | 0x20 | 4 |
| 故障 | 0x0D | 5 |

Word 表12将 `S_Mode` 写成bit13～14、bit15保留，**与结构布局不符**。当前 `S_Mode` 实际在bit14～15，掩码0xC000；bit11～13保留。该旧自动/手动模式字段也不等于现代开环/闭环 `ac_mode`。

`PORT_STATUS` bit3/4 当前由 `GNDSwitch/ISOSwitch` 填入，原先基于外部断路器反馈的赋值被注释，不能按文档直接显示为互补的“外部断路器合/分”。bit7～9仍以1表示闭锁，而新扩展PWM标志以1表示使能，两者极性相反。

证据：`SstWireTypes.h:6`、`:27`、`DPRAM.C:3067`、`ChbDabControl.c:1061` 起。

### 5.3 PORT_FAULT

Word表12的通用定义与当前 HVAC 赋值不完全一致：

| 位 | Word含义 | 当前HVAC实际用途 |
|---|---|---|
| bit10 | 过流 | 硬件端口过流，来自FaultTypeFLAG |
| bit12 | 过压 | 硬件端口过压，来自FaultTypeFLAG |
| bit13 | 通讯 | 原软件过压 HVAC_OV_Fault |
| bit14 | SYNC | 原软件过流 HVAC_OC_Fault |
| bit15 | 端口有故障 | 根据 `(PORT_FAULT & 0x75FF) != 0` 生成；掩码不含bit9和bit11 |

**现代快速软件过流/过压、预充超时、PLL超时等不保证产生旧PORT_FAULT原因位。旧位图为0不能据此宣布系统无故障。** 必须同时读取扩展控制故障 `0x66002E08`：0无、1预充超时、2PLL超时、3快速交流过流、4快速直流过压、5硬件、6非法配置、7开关反馈故障。

五位详细故障码表在 `0x66002E40` 起；`0x66002E10` 是当前故障表首项，不是“首次跳闸原因”的永久锁存。历史区保存后台记录的故障上下文。

证据：`FaultCheck.C:252`、`SstPortStatus.c:161`、`SstFault.c:48`、`ChbDabControl.h:53`。

## 6 文档缺失的当前并口扩展

该扩展位于旧插件区之后，不覆盖旧1～20块的有效数据区和邮箱。不是RS485的A5 5A帧。

| 区域 | DSP首址 | 逻辑字首址 | 长度 | ARM权限 |
|---|---|---|---:|---|
| 请求 | 0x66002C00 | 0x1600 | 128字，前16字为请求 | 写 |
| 回复及参数实际值 | 0x66002D00 | 0x1680 | 128字 | 读 |
| 现代状态及遥测 | 0x66002E00 | 0x1700 | 128字 | 读 |
| 详细故障历史 | 0x66002F00 | 0x1780 | 512字 | 读 |

主机写完整请求，最后写提交标记，再写左邮箱21。请求含 magic=0x5353、version=1、非零序号、操作、点号/值、命令字、CRC16、序号副本和commit=0xA55A。op0读点、op1写点、op2五字遥控、op3保存、op4加载；保存/加载的value还须填0xA55A。

回复/遥测/历史分别使用发布版本戳：读前后word0必须相同且为非零偶数。回复还要检查ack_sequence、ack_result；保存必须继续检查storage_result和pending，不能把请求受理等同保存完成。同序号同载荷只缓存最近一笔结果，不应任意重放旧请求。

54点使用逐点定义的16位定点编码，**不使用Word第6节的两个16位字传float方式**。关键点如下，表中地址仅为实际值回读；写入统一走请求区point/value，不能直接写这些回读地址。

| 点号 | 参数 | 实际值回读地址 | 解码 |
|---:|---|---|---|
| 6 | 单模块DC目标 dc_voltage_ref_v | 0x66002D4C | uint16 / 10 V |
| 7 | iq_reference_a | 0x66002D4E | int16 / 100 A |
| 11 | 状态机预充门槛 | 0x66002D56 | uint16 / 10 V |
| 24 | 电压Kp | 0x66002D70 | int16 / 100000 |
| 25 | 电压Ki | 0x66002D72 | int16 / 100 |
| 33 | 电流Kp | 0x66002D82 | int16 / 1000 |
| 34 | 电流Ki | 0x66002D84 | int16 / 1000 |
| 39 | 快速过流门槛 | 0x66002D8E | uint16 / 100 A |
| 40 | 快速过压门槛 | 0x66002D90 | uint16 / 10 V |
| 46 | 原RMS软件过流阈值 | 0x66002D9C | uint16 A |
| 50 | 原RMS软件过压阈值 | 0x66002DA4 | uint16 V |
| 52 | 旧模块预充筛查阈值 | 0x66002DA8 | uint16 / 10 V |
| 53 | 旧模块高压筛查阈值 | 0x66002DAA | uint16 / 10 V |

完整请求、回复、参数权限及保存规则见：

- [DPRAM_EXTENSION.md](/D:/Code/SST/AnyEyesVersionV1/docs/DPRAM_EXTENSION.md)
- [DPRAM_PARAMETERS.csv](/D:/Code/SST/AnyEyesVersionV1/docs/DPRAM_PARAMETERS.csv)
- [DPRAM_MAPPING.md](/D:/Code/SST/AnyEyesVersionV1/docs/DPRAM_MAPPING.md)
- [DPRAM_BITS.csv](/D:/Code/SST/AnyEyesVersionV1/docs/DPRAM_BITS.csv)

旧文档没有这套点表，若ARM只实现文档中的块1/2/3，无法完整维护现代PI、保护、预充及运行参数。当前 `CHB_LOAD_EEPROM_PARAMETERS=0`，显式保存成功也不代表下次上电默认自动恢复；上电恢复或显式加载策略需单独约定。

## 7 插件块及LAN边界

U7、U11、U15在当前轮询中跳过；U17只发块19通知，数据复制已注释。不能把文档“4～19插件镜像”理解为所有插件均持续刷新。其他插件也每完整轮询更新一次，并非每30ms全部更新。

Word第6节A5 5A维护帧、Hello/对象索引0x01～0x08、115200波特率不是当前DSP接口。当前UART默认460800、VOFA；选择AnyEyes时接收0x6B请求。旧透明寄存器测试入口已明确禁止，不能照第7节直接将0x10/0x03/0x05帧透传给当前UART而期待兼容。

本文不能确认STM32 FSMC基址、N510配置、THOST/ARM端TCP封装或字节序实现；没有提供这些工程。若LAN由ARM服务转成本文DPRAM操作，可按新旧点表实现适配；若LAN只是N510透明转485，则必须匹配当前AnyEyes或VOFA协议。

## 8 验证与建议

本次运行了现有 `tests/dpram_commands.py` 和 `tests/dpram_maintenance.py`：两套各4种模式均通过，覆盖控制源互锁、STOP/OFF优先、停机/存储忙校验，以及扩展CRC/提交标记/序号、回读、保存和分片发布。测试替换了硬件边界，不验证ARM电气时序、实际邮箱中断、串口/LAN连接或功率回路。

建议保留当前旧地址兼容层，修订Word：旧块仅列明实际可用字段；现代维护统一补充邮箱21及扩展区；改正状态和故障位定义；将A5 5A协议标为历史版本。ARM界面读取现代状态、故障码和Udc，现代参数经扩展点表写入并检查结果。单独合分闸、多端口模式、PT/CT配置等若确需支持，应另做明确的受控功能设计，不能仅启用旧函数绕过当前状态机。
