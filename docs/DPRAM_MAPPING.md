# DPRAM 模式并口地址与信号映射

本文于 2026-10-13 核对，基于提交 `b151f12962a1d9f5d716f17606c65c93ec978b91` 的源码。对应 `D:\Code\SST\AnyEyesVersion` 当前 `HVAC_online` 构建，记录 `SST_ENABLE_DPRAM=1` 时的固定 DPRAM 布局、软件协议方向、数值缩放和实际功能接入情况。

完整逐字地址见 [DPRAM_ADDRESSES.csv](DPRAM_ADDRESSES.csv)，逐位定义见 [DPRAM_BITS.csv](DPRAM_BITS.csv)。CSV 同时列出 DSP 字节地址、相对字节偏移及16位字地址，并标记仅缓存、无生产者、轮询跳过等状态。本文描述当前实现；本次整理没有修改固件或补接缺少的接口。

## 使用前提

表中的地址是 **C6748 DSP 侧的 EMIFA/DPRAM 字节地址**，不是外部主机自身的物理地址。C6748 按字节寻址；本 DPRAM 协议按16位半字访问，每字段占2字节，字段间地址递增2。工程使用小端模式，拆为字节时低字节在前。主机应根据自己的并口片选映射，把以下 DSP 地址换算成主机侧地址；不能直接假定主机也使用 `0x66000000` 基址。

| 项目 | 定义 |
|---|---|
| DSP DPRAM 基址 | `0x66000000` |
| 16 位字地址偏移 | `(DSP字节地址 - 0x66000000) / 2` |
| 主机通知 DSP | 写左邮箱 `0x66003FFC`，值为块号 |
| DSP 通知主机 | DSP 写右邮箱 `0x66003FFE`，值为块号 |
| 权限说明 | 下表 R/W 是当前软件协议方向；源码没有字段级只读保护。外部主机实际总线访问能力以硬件设计为准 |
| DPRAM 使能 | `SST_ENABLE_DPRAM=1` |
| 远程控制/遥调附加条件 | `SST_SIMULATOR_MODE=0`；默认仿真器模式为 `1`，此时遥测可运行，但 DPRAM 启停、封波、复位和遥调提交均被门控 |

固定地址来自 [`C6748.cmd`](../C6748.cmd)、[`DpramConfig.h`](../Source/02_Peripheral/01_DPRAM/DpramConfig.h) 和 [`DPRAM.H`](../Source/02_Peripheral/01_DPRAM/DPRAM.H)。链接审计还验证了这些区域为 `NOINIT`，不会由 C 运行库写入启动镜像。

数值字段的 `short` 为16位有符号二补码（-32768..32767），`ushort`/位图为16位无符号（0..65535）。浮点RAM缓存没有直接暴露到并口，旧遥测使用 `(short)` 转换，截去小数；当前旧打包没有饱和处理，主机应按有符号字段及其缩放解码，不能一律按无符号值解释。

## 区域和邮箱地址

| 区域 | DSP 首地址 | 字偏移首地址 | 长度 | 主机软件方向 | 用途 |
|---|---:|---:|---:|---|---|
| Block1 遥控 | `0x660001E0` | `0x00F0` | 16 字（32 B） | W：主机写，DSP读 | 启停、独立 CHB/DAB 脉冲、故障复位 |
| Block2 遥调 | `0x660003FE` | `0x01FF` | 43 字（86 B） | R/W：主机写，DSP回写同一块 | 旧端口遥调布局；当前只有两个旧字段有执行效果 |
| Block3 公共遥测 | `0x66000A00` | `0x0500` | 33 字（66 B） | R：主机读 | 状态、旧故障位图、端口遥测 |
| Block4 U2 | `0x66000D00` | `0x0680` | 128 字（256 B） | R：主机读 | 插件 U2 原始采样/状态 |
| Block4 U3…U17 | `0x66000D00+(n-2)*0x200` | `0x0680+(n-2)*0x100` | 每块128字 | R：主机读 | 插件原始采样/状态 |
| 左邮箱 | `0x66003FFC` | `0x1FFE` | 1 字 | W：主机写 | 写 `1` 请求读 Block1，写 `2` 请求读 Block2 |
| 右邮箱 | `0x66003FFE` | `0x1FFF` | 1 字 | R：主机读 | DSP 回报 `3`、`4..19` 或 `20` |

邮箱不是带序号的可靠事务协议。没有 CRC、请求序号、ACK、错误码、队列和超时回报。主机应先完整写入 16 字或 43 字，再写左邮箱；DSP 读取后不会清零遥控块。每次请求都应把未使用的命令字清零，否则上一次的 START/ON/STOP/OFF/RESET 位可能被重放。Block2 的主机写入区和 DSP 回读区共用同一内存，回读可能覆盖主机刚写入的数据。

左邮箱和中断待处理标志均只保留最后一次通知，前一请求未消费前连续写入新块号可能覆盖请求。主机应逐笔提交，等待可观察的状态变化或遥调回读后再提交；并避免与DSP回写同一Block2同时访问。

GPIO101 的 DPRAM 中断只置 `Flag_RemoteControlRegulate`，后台再执行搬运和处理，见 [`Redef_Interrupt.c`](../Source/03_System/02_ISR/Redef_Interrupt.c#L598) 和 [`SstDpramHost.c`](../Source/04_App/10_Host/SstDpramHost.c#L21)。Timer2 每 3 个 10 ms 节拍置遥测标志，名义周期为 30 ms；每轮先发送 Block3，再发送一个插件块或 Block2 回读块。

每轮连续两次写右邮箱，中间没有等待主机确认，后一个块号可能覆盖 `3` 的通知；读取右邮箱并不能获得完整更新队列。公共块名义每30 ms刷新一次，单插件要走完14个轮询位置才再次刷新，名义约420 ms，实际还受后台处理延时影响。搬运期间没有快照/版本号，不能保证多字数据原子一致。以上均为软件逻辑核对，未在本次执行实板并口或100 µs中断时序测量。

正常轮询右邮箱块号顺序为 `4,5,6,7,8,10,11,12,14,15,16,18,19,20`，每个位置前还会通知公共块 `3`；块20只有遥调回写标志置位才产生通知，插件块号等于 `U编号+2`。U7/U11/U15被跳过，U17只有通知没有复制。

## Block1：遥控地址表

Block1 首地址为 `0x660001E0`，每个字段一个 16 位字。协议方向是主机 W；DSP 可读共享内存，但不会把执行结果写回命令字段。

| word | DSP地址 | 字段 | 位定义 | 当前入口 |
|---:|---:|---|---|---|
| 0 | `0x660001E0` | `Reserved_1[0]` | 保留 | 不消费 |
| 1 | `0x660001E2` | `Reserved_1[1]` | 保留 | 不消费 |
| 2 | `0x660001E4` | `COM_START_PORT` | bit0 HVAC；bit1 HVDC；bit2 LVDC；bit3 LVAC；bit4..15 保留 | **仅 bit0 有效**：同时请求 CHB 和 DAB 运行 |
| 3 | `0x660001E6` | `COM_STOP_PORT` | bit0 HVAC；bit1 HVDC；bit2 LVDC；bit3 LVAC；bit4..15 保留 | **仅 bit0 有效**：撤销 CHB 和 DAB 运行 |
| 4 | `0x660001E8` | `COM_PORT_MODE` | bit0 恒压；bit1 恒功率；bit2 下垂；bit3 跟随；bit4/5 注入谐波使能/禁止；bit6/7 治理谐波使能/禁止；bit8/9 STATCOM 电压使能/禁止；bit10/11 功率因数使能/禁止；bit12/13 无功使能/禁止；bit14/15 保留 | 有布局，无当前消费 |
| 5 | `0x660001EA` | `Reserved_2` | 保留 | 不消费 |
| 6 | `0x660001EC` | `COM_SWITCH_ON` | bit0 内部接触器；bit1 内部断路器；bit2 外部断路器1；bit3 外部断路器2；其余保留 | 有布局，无当前消费 |
| 7 | `0x660001EE` | `COM_SWITCH_OFF` | bit0..3 与 `COM_SWITCH_ON` 相同，表示分闸请求 | 有布局，无当前消费 |
| 8 | `0x660001F0` | `COM_PLUSE_ON` | bit0 HF/DAB 解锁；bit1 LF/CHB 解锁；bit2..15 保留 | **有效** |
| 9 | `0x660001F2` | `COM_PLUSE_OFF` | bit0 HF/DAB 闭锁；bit1 LF/CHB 闭锁；bit2..15 保留 | **有效** |
| 10 | `0x660001F4` | `COM_FAULT_CLEAR` | bit0=1 请求清故障；bit1..15 保留 | **有效**，仅完全停机时接受 |
| 11 | `0x660001F6` | `PORT_SWITCH_EN` | bit0 外部断路器合闸允许；bit1 合闸禁止；bit2..15 保留 | 有布局，无当前消费 |
| 12 | `0x660001F8` | `PORT_NAME` | bit0 HVAC；bit1 HVDC；bit2 LVDC；bit3 LVAC；bit4..15 保留 | 有布局，无当前消费 |
| 13 | `0x660001FA` | `PORT_SENSORTYPE` | bit0/1 PT变比1/2；bit2/3 PT接线1/2；bit4/5 CT变比1/2；bit6/7 CT接线1/2；bit8..15 保留 | 有布局，无当前消费 |
| 14 | `0x660001FC` | `PORT_GRIDTYPE` | bit0 电网；bit1 负载；bit2 强网；bit3 弱网；bit4 电网故障；bit5 电网正常；bit6..15 保留 | 有布局，无当前消费 |
| 15 | `0x660001FE` | `Reserved_3` | 保留 | 不消费 |

实际入口只把 word 2、3、8、9、10 传给 [`SstCommand_ApplyDpram`](../Source/04_App/04_Control/SstCommand.c#L90)。STOP bit0优先并立即返回，因此同帧RESET不会执行；OFF 优先于对应 ON，而且任何有效OFF位都会抑制同帧START；参数存储忙时 STOP/OFF 仍执行，而 START/ON/RESET 被抑制。RESET 还要求控制状态为 STOPPED 或 FAULT、两路命令为 0、两路实际 PWM 均关闭。旧的 `RemoteControl_Dispose()` 虽然仍在 `DPRAM.C`，当前 DPRAM 后台入口不调用它。

以下仅说明请求编码，不绕过状态机、硬件故障或预充/PLL条件：每次先把16字全部置零，再设置所列字，最后写左邮箱为 `1`。

| 操作 | 设置的命令字 | 数值 |
|---|---|---:|
| 请求CHB+DAB启动 | `0x660001E4` | `0x0001` |
| 请求CHB+DAB停止 | `0x660001E6` | `0x0001` |
| 设置CHB独立启动请求，DAB保持现状 | `0x660001F0` | `0x0002` |
| 设置DAB独立启动请求，CHB保持现状 | `0x660001F0` | `0x0001` |
| 仅关DAB | `0x660001F2` | `0x0001` |
| 仅关CHB | `0x660001F2` | `0x0002` |
| 关CHB和DAB | `0x660001F2` | `0x0003` |
| 停机后另发故障复位 | `0x660001F4` | `0x0001` |

状态反馈示例：从 `0x66000A00` 读到的字按 `word & 0x003F` 取旧状态；`0x20`为运行，`0x0D`为当前核心故障态映射。`0x66000A02` 的 bit8/bit9 为CHB/DAB闭锁状态，1闭锁、0解锁。命令写入共享内存后可物理读回，但该读回不是执行确认。

命令位写0表示不动作，不会撤销已提交的运行请求。要关闭另一通道，必须另设对应OFF位或发送STOP；清零共享命令块仅用于避免重放。

## Block2：遥调地址表

Block2 首地址为 `0x660003FE`，地址计算为 `0x660003FE + 2*word`。本表中的 `R/W` 表示 DSP 会在旧遥调接受后回写同一块；不代表每个字段都有有效的现代控制消费逻辑。

| word | DSP地址 | 字段 | 线上数值/位定义 | 当前写入效果 |
|---:|---:|---|---|---|
| 0 | `0x660003FE` | `COM_SYS_MODE` | bit0 HVAC 1运行/0测试；bit1 HVAC 1自动/0手动；bit2/3 HVDC；bit4/5 LVDC；bit6/7 LVAC；bit8..15保留 | 仅缓存，应用代码注释 |
| 1 | `0x66000400` | `AD_PROT_EN` | bit12 A相保护；bit13 B相；bit14 C相；1使能、0禁止；bit0..11及bit15在HVAC为保留 | 仅缓存，应用代码注释 |
| 2 | `0x66000402` | `FO_1_EN` | bit0..14 为第1组 FO 通道使能，bit15保留 | 仅缓存 |
| 3 | `0x66000404` | `FO_2_EN` | 同上，第2组 | 仅缓存 |
| 4 | `0x66000406` | `FO_3_EN` | 同上，第3组 | 仅缓存 |
| 5 | `0x66000408` | `FB_EN` | bit0..15 对应原插件2..17，1使能、0禁止 | 仅缓存 |
| 6 | `0x6600040A` | `PORT_P_SET` | `int16`；旧有功给定 | 转换代码注释 |
| 7 | `0x6600040C` | `PORT_Q_SET` | `int16`；旧无功给定 | 转换代码注释 |
| 8 | `0x6600040E` | `PORT_V_SET` | `int16`，1 count=1 V；600..750 V接受，越界改写为700 | **有效于旧端口缓存**；不更新新核心 `dc_voltage_ref_v` |
| 9 | `0x66000410` | `PORT_F_SET` | 旧格式代码为 raw/10 Hz | 转换代码注释 |
| 10..12 | `0x66000412..0x66000416` | `RMS_CURRENT_5TH/7TH/11TH_SET` | 旧格式代码为 raw/10 | 转换代码注释 |
| 13..15 | `0x66000418..0x6600041C` | `PHA_CURRENT_5TH/7TH/11TH_SET` | 旧格式代码为 raw/1000 | 转换代码注释 |
| 16 | `0x6600041E` | `STATCOM_V_SET` | 旧格式 1 count=1 V | 转换代码注释 |
| 17 | `0x66000420` | `STATCOM_PF_SET` | 旧格式 raw/100 | 转换代码注释 |
| 18 | `0x66000422` | `STATCOM_Q_SET` | `int16`；旧浮点缓存写入 `raw*100`；同时向 FPGA `0x6000062C` 写 `raw/200`（整数除法） | **有效于旧缓存/FPGA寄存器** |
| 19 | `0x66000424` | `PORT_V0_SET` | 旧下垂电压截距 | 转换代码注释 |
| 20 | `0x66000426` | `PORT_F0_SET` | 旧格式 raw/10 | 转换代码注释 |
| 21 | `0x66000428` | `PORT_KF_SET` | 旧格式 raw/10 | 转换代码注释 |
| 22 | `0x6600042A` | `PORT_KV_SET` | 旧格式 raw/10 | 转换代码注释 |
| 23..26 | `0x6600042C..0x66000432` | `Reserved_1[0..3]` | 保留/旧兼容字段 | 当前应用注释 |
| 27 | `0x66000434` | `PORT_V_P_1` | 旧 PI 格式，原代码按 `/100` 解释 | **PI应用注释** |
| 28 | `0x66000436` | `PORT_V_I_1` | 旧 PI 格式，原代码按 `/100` 解释 | **PI应用注释** |
| 29 | `0x66000438` | `PORT_I_P_1` | 旧 PI 格式，原代码按 `/100` 解释 | **PI应用注释** |
| 30 | `0x6600043A` | `PORT_I_I_1` | 旧 PI 格式，原代码按 `/100` 解释 | **PI应用注释** |
| 31..34 | `0x6600043C..0x66000442` | `PORT_V_P/I_2`、`PORT_I_P/I_2` | 同上，旧第二组 PI | **PI应用注释** |
| 35..38 | `0x66000444..0x6600044A` | `PORT_V_P/I_3`、`PORT_I_P/I_3` | 同上，旧第三组 PI | **PI应用注释** |
| 39..42 | `0x6600044C..0x66000452` | `EER2_DATA[0..3]` | 原 EER2 兼容字 | 当前应用注释 |

Block2 只有完全停机且参数存储不忙时才进入旧 `RemoteRegulate_Dispose_2(0)`。当前 HVAC 分支中系统模式、AD/FO/FB 使能、P/Q/F、谐波、模块配置和 PI 处理段均被注释；函数也没有向 `g_chb_dab_command` 或 `g_chb_dab_params` 赋值。因此 `PORT_V_SET` 和 `STATCOM_Q_SET` 不能当作迁移后 CHB/DAB 闭环给定。新核心实际读取的是 `g_chb_dab_command.dc_voltage_ref_v` 和 `iq_reference_a`。

收到满足处理条件的遥调后，旧函数还会同步调用 EEPROM Page1 写入，保存旧V_SET/Q_SET各2字节，**这段写入不受 `EEPROM_ON` 门控**；随后置 `FLAG_RemRegulate_SetOver`。只有该标志置位时，后台轮询到位置20才回写整块43字并通知右邮箱 `20`；启用 `EEPROM_ON` 时，回写前还会从旧EEPROM Page1/Page2恢复缓存。该旧写入路径不等同于新参数存储服务，`20`不表示所有43字段已应用，更不表示现代控制参数修改成功。上位机在运行或保存忙时下发块2，DSP仍先搬入缓存再拒绝处理，没有失败返回码。

## Block3：公共遥测地址表

Block3 首地址为 `0x66000A00`，DSP 写、主机读。下表的缩放是当前 HVAC `RemoteSig_Dispose()` 实际写入的缩放。未赋值字段可能保留旧 RAM 内容或为零，不能当作有效实时量。

| word | DSP地址 | 字段 | 当前含义和数值定义 |
|---:|---:|---|---|
| 0 | `0x66000A00` | `PORT_RUN_STATUS` | 16 位主状态字，见下一节 |
| 1 | `0x66000A02` | `PORT_STATUS` | 16 位开关/PWM状态字，见下一节 |
| 2 | `0x66000A04` | `PORT_FAULT` | 16 位旧故障位图，见下一节；不是191xx/291xx数字码 |
| 3 | `0x66000A06` | `PORT_P` | `PORT_RealTime_DATA.PORT_P * 10`，0.1 kW/count |
| 4 | `0x66000A08` | `PORT_Q` | `PORT_RealTime_DATA.PORT_Q * 10`，0.1 kvar/count |
| 5 | `0x66000A0A` | `PORT_V_A` | AB 线电压 RMS，整数 V/count |
| 6 | `0x66000A0C` | `PORT_V_B` | BC 线电压 RMS，整数 V/count |
| 7 | `0x66000A0E` | `PORT_V_C` | CA 线电压 RMS，整数 V/count |
| 8 | `0x66000A10` | `PORT_I_A` | A相端口电流 RMS，0.1 A/count |
| 9 | `0x66000A12` | `PORT_I_B` | B相端口电流 RMS，0.1 A/count |
| 10 | `0x66000A14` | `PORT_I_C` | C相端口电流 RMS，0.1 A/count |
| 11 | `0x66000A16` | `PORT_I_N` | HVAC 打包函数未更新，不是有效中线电流 |
| 12..14 | `0x66000A18..0x66000A1C` | `V_A_2/V_B_2/V_C_2` | HVAC 打包函数未更新，不是有效第二组电压 |
| 15..17 | `0x66000A1E..0x66000A22` | `I_A_2/I_B_2/I_C_2` | 第二组/网侧电流 RMS，0.1 A/count |
| 18 | `0x66000A24` | `DC_V` | HVAC 打包函数未更新，不能当作当前直流母线电压 |
| 19..21 | `0x66000A26..0x66000A2A` | `RMS_CURRENT_5TH/7TH/11TH` | 旧格式虽有乘10代码，但当前 HVAC 没有有效生产者 |
| 22..24 | `0x66000A2C..0x66000A30` | `PHA_CURRENT_5TH/7TH/11TH` | 旧格式虽有乘1000代码，但当前 HVAC 没有有效生产者 |
| 25 | `0x66000A32` | `PORT_PF` | 旧格式乘100；当前 HVAC 无有效生产者 |
| 26..29 | `0x66000A34..0x66000A3A` | `PORT_V0/F0/KF/KV` | 旧下垂量；当前 HVAC 无有效生产者 |
| 30..32 | `0x66000A3C..0x66000A40` | `Reserved_1[0..2]` | 旧兼容保留字段，不能当作新运行量 |

word3..32均按 `int16` 布局，不包含浮点位模式。示例：word8读到 `123` 表示12.3 A，读到 `0xFF9C` 应按 -100 解码后得到 -10.0 A（字段是否允许出现负值取决于其生产者）。完整CSV把上述范围逐字展开，并保留当前未赋值字段，便于主机严格核对布局。

## 状态字定义

### `PORT_RUN_STATUS`（Block3 word0）

位域为：bits5:0=`P_Status`，bit6=`Sys_Stop`，bit7=`Outer_Elec`，bits9:8=`P_S2_Mode`，bits11:10=`P_S1_Mode`，bits13:12=`P_M_Mode`，bits15:14=`Label`。

当前新控制状态机写入的低6位映射如下；这是旧端口状态码，不是现代 `CHB_STATE` 的 0..5 直接值：

| 现代状态 | `CHB_STATE`内部值 | DPRAM `P_Status` |
|---|---:|---:|
| STOPPED | 0 | `0x06` Standby |
| PRECHARGE | 1 | `0x10` Start |
| BYPASS_DELAY | 2 | `0x14` ContactorOnOver |
| WAIT_PLL | 3 | `0x16` PortStart |
| RUNNING | 4 | `0x20` Run |
| FAULT | 5 | `0x0D` Stop |

旧常量的完整值为：`0x05 Initial`、`0x07 PluseClose`、`0x06 Standby`、`0x20 Run`、`0x3F Nonvalid`、`0x10 Start`、`0x09 WaitPowerStart`、`0x11 Start_SWTable`、`0x12 Start_SWOver`、`0x13 Start_ContactorOn`、`0x14 Start_ContactorOnOver`、`0x15 Start_DABDutyStart`、`0x16 Start_PortStart`、`0x17 Start_JudgeStatus`、`0x21 Start_SVGRun`、`0x0D Stop`、`0x0E Stop_EXTBreakerOff`、`0x0F Stop_ContactorOff`、`0x0A ManualStart_PortStart`、`0x0B ManualStart_SVGStart`、`0x0C ManualStart_SWOver`。这些是兼容定义，当前现代核心正常映射只使用上表6个值；不要照共用头文件中的旧状态注释解码。

bits9:8 的附加模式二：0无、1注入谐波、2治理谐波；bits11:10 的附加模式一：0无、1无功、2功率因数、3电压；bits13:12 的主模式：0恒压、1恒功率、2下垂、3跟随。它们不是新核心的 `ac_mode=0` 开环、`ac_mode=1` 闭环。

### `PORT_STATUS`（Block3 word1）

当前 HVAC 打包函数的实际赋值如下：

| 位 | 当前含义 |
|---:|---|
| 0 | 内部接触器，0断、1合 |
| 1 | 内部断路器合反馈 |
| 2 | 内部断路器分反馈 |
| 3 | 当前实际复用为 `GNDSwitch` |
| 4 | 当前实际复用为 `ISOSwitch` |
| 5 | 定义为 `GND_Switch`，当前打包函数未更新 |
| 6 | 定义为 `ISO_Switch`，当前打包函数未更新 |
| 7 | 模块H桥 PWM 状态，1闭锁、0解锁 |
| 8 | 工频 CHB PWM 状态，1闭锁、0解锁 |
| 9 | 高频 DAB PWM 状态，1闭锁、0解锁 |
| 10 | 外部断路器合闸允许 |
| 11..13 | 保留 |
| 14..15 | 旧 `S_Mode`：0自动运行、1自动测试、2手动运行、3手动测试 |

### `PORT_FAULT`（Block3 word2）

这是 `Port_RealFault.PORT_FAULT16` 的旧16位位图，实际含义以当前 HVAC 代码为准：

| 位 | 当前含义 |
|---:|---|
| 0 | 接触器异常 |
| 1 | 断路器异常 |
| 2 | 风机故障 |
| 3 | 控制器故障 |
| 4 | 辅助供电故障 |
| 5 | EPO急停 |
| 6 | 模块汇总故障 |
| 7 | 工频 CHB 桥故障 |
| 8 | 相序错误 |
| 9 | 频率越限 |
| 10 | 硬件端口过流 |
| 11 | 模块欠压 |
| 12 | 硬件端口过压 |
| 13 | 原软件过压 `HVAC_OV_Fault` |
| 14 | 原软件过流 `HVAC_OC_Fault` |
| 15 | 汇总故障；由 `(PORT_FAULT & 0x75FF) != 0` 生成 |

头文件中 bit13/14 的通用注释仍写作通信/SYNC，但 HVAC 运行代码已将它们用于原软件过压/过流。新快速保护、预充超时、PLL超时、非法配置只进入现代 `g_chb_dab_status.fault_code` 和 FAULT 状态，没有写入 Block3 的数字故障码或对应原因位。因此 DPRAM 可能读到 `P_Status=0x0D`，但无法仅靠 Block3 区分具体快速保护原因。

汇总掩码 `0x75FF` 不包含 bit9（频率）和 bit11（欠压）；bit15不能理解为bit0..14的全部按位或。快速保护故障可能出现 `P_Status=0x0D` 而 `PORT_FAULT=0`；读取旧故障位图为0不能据此断言控制核心无故障。

## Block4：插件原始遥测地址

每个插件块128字，DSP 写、主机读，不做定点转换。FPGA 槽结构中 `AD[16]` 和 `AD_LATCH[16]` 位于槽内前半段；DPRAM 只从槽内 `+0x100` 的 DC 区开始复制，因此原始 AD/AD_LATCH 不在这些 DPRAM 插件块中。

| DPRAM块 | DSP地址 | FPGA源地址 | 当前刷新 |
|---|---:|---:|---|
| U2 | `0x66000D00` | `0x60000500` | 每轮轮询 |
| U3 | `0x66000F00` | `0x60000100` | 每轮轮询 |
| U4 | `0x66001100` | `0x60000300` | 每轮轮询 |
| U5 | `0x66001300` | `0x60000700` | 每轮轮询 |
| U6 | `0x66001500` | `0x60000900` | 每轮轮询 |
| U7 | `0x66001700` | `0x60000B00` | **轮询跳过** |
| U8 | `0x66001900` | `0x60000D00` | 每轮轮询 |
| U9 | `0x66001B00` | `0x60000F00` | 每轮轮询 |
| U10 | `0x66001D00` | `0x60001100` | 每轮轮询 |
| U11 | `0x66001F00` | `0x60001300` | **轮询跳过** |
| U12 | `0x66002100` | `0x60001500` | 每轮轮询 |
| U13 | `0x66002300` | `0x60001700` | 每轮轮询 |
| U14 | `0x66002500` | `0x60001900` | 每轮轮询 |
| U15 | `0x66002700` | `0x60001B00` | **轮询跳过** |
| U16 | `0x66002900` | `0x60001D00` | 每轮轮询 |
| U17 | `0x66002B00` | 无有效复制源 | 发送块19通知，但数据复制语句已注释，内容不刷新 |

每块内部均为 16 位原始字，字段偏移相对该块首地址如下：

表中“每轮轮询”指每完整轮询一遍，不是每30 ms都更新该插件。U2/U3/U4沿用旧兼容重排：分别复制原FPGA槽4/槽2/槽3的后半段；字段是否有有效硬件生产者取决于插件类型。不能把每个块里的DC/IG/TEMP都解释为有效功率模块采样。

| word偏移 | 字段 | 数值定义 |
|---:|---|---|
| 0..7 | `DC[8]` | 原始模块直流电压，当前换算代码按 1 V/count 使用 |
| 8..15 | `DC_LATCH[8]` | 直流电压锁存原始值 |
| 16..23 | `IR[8]` | 原始直流电流；当前 DSP换算为 raw/10 A |
| 24..31 | `IR_LATCH[8]` | 直流电流锁存原始值 |
| 32..39 | `IG[8]` | 原始端口电流；当前 DSP换算为 raw/10 A |
| 40..47 | `IG_LATCH[8]` | 端口电流锁存原始值 |
| 48..55 | `TEMP[8]` | 原始温度；当前 DSP换算为 raw/10 工程值 |
| 56..63 | `TEMP_LATCH[8]` | 温度锁存原始值 |
| 64..71 | `STATUS[8]` | 模块状态字 |
| 72..79 | `HD_S_PH[8]` | 旧 HVDC 同步相位原始字；当前 HVAC 无有效换算 |
| 80..87 | `DRIVER[8]` | 四路驱动状态压缩字 |
| 88..95 | `Reserved_6[8]` | 保留 |
| 96 | `DIN` | 开入位图 |
| 97 | `DIN_LATCH` | 开入锁存位图 |
| 98 | `AD_PROT` | AD保护实时位图 |
| 99 | `AD_PROT_LATCH` | AD保护锁存位图 |
| 100 | `FO` | 光纤故障实时位图 |
| 101 | `FO_LATCH` | 光纤故障锁存位图 |
| 102 | `FB` | bit0..15 对应原插件2..17 |
| 103 | `FB_LATCH` | FB锁存位图 |
| 104..111 | `Reserved_7[8]` | 保留 |
| 112 | `OR_STATUS` | 模块 STATUS 按位或 |
| 113..115 | `Reserved_8[3]` | 保留 |
| 116 | `Left_Ref` | 左桥原始参考计数 |
| 117 | `Right_Ref` | 右桥原始参考计数 |
| 118..119 | `Reserved_9[2]` | 保留 |
| 120 | `INV_FO` | 独立逆变桥 FO 位图 |
| 121 | `INV_FO_LATCH` | 独立逆变桥 FO 锁存位图 |
| 122 | `Sum_DC` | 模块直流电压和，原始 16 位字 |
| 123 | `Avg_DC` | 模块直流电压平均值，原始 16 位字 |
| 124 | `Sum_IG` | 模块端口电流和，原始 16 位字 |
| 125 | `Avg_IG` | 模块端口电流平均值；当前换算代码按 raw/10 A 使用 |
| 126 | `Sum_IR` | 备用通道电流和，原始字 |
| 127 | `Avg_IR` | 备用通道电流平均值，原始字；当前无有效换算消费 |

`STATUS` 的 bits0..13 依次为 `OV、OC2、OC1、OT、SVG_H、DAB_H、UV、SHUT、OV_L、OC2_L、OC1_L、OT_L、SVG_L、DAB_L`，bits14..15 为 Label。`DRIVER` 四组分别为bits2:0、5:3、8:6、11:9，7表示正常、2表示上管故障、4表示下管故障、6表示驱动板过压/过温/母线异常、0表示上电配置故障；bits13:12保留、bits15:14为Label。

插件 `DIN/DIN_LATCH` 的原位图：bit0外部断路器分、bit1外部断路器合、bit2第二外部断路器分、bit3第二外部断路器合、bit4内部断路器分、bit5内部断路器合、bit6保留、bit7内部接触器、bit8..15无效/保留。FO/AD_PROT/INV_FO为原始硬件通道位图，通道意义依赖插件类型，不能套用公共 `PORT_FAULT` 位定义。

## 用户关注参数的接入结论

| 项目 | DPRAM 当前结论 | 可用替代接口 |
|---|---|---|
| 启动、停机 | **已接入**，Block1 word2/3 bit0 | DPRAM模式由DPRAM提交；AnyEyes普通启停在此模式被拒绝 |
| CHB 独立解锁/封波 | **已接入**，word8/9 bit1 | DPRAM |
| DAB 独立使能/封波 | **已接入**，word8/9 bit0 | DPRAM |
| 故障复位 | **已接入**，word10 bit0，完全停机才接受 | DPRAM；AnyEyes普通复位在此模式被拒绝 |
| 旧状态码/PWM状态 | **已接入**，Block3 word0/1 | 读取 Block3 |
| 旧故障位图 | **已接入**，Block3 word2 | 读取 Block3 |
| 191xx/291xx 数字故障码、等级、首故障、历史 | **未映射到 DPRAM** | AnyEyes/SstFault 接口 |
| 当前 CHB_STATE、启动阶段、锁相状态 | **未提供独立 DPRAM 字段**；仅映射为旧 `P_Status` | AnyEyes状态显示 |
| 新核心 DC 给定、Iq给定、开环调制/频率、`ac_mode` | **未接入 DPRAM** | AnyEyes固定参数组 |
| 新核心 PLL/电压环/电流环 PI | **未接入 DPRAM**；Block2旧PI字段仅缓存 | AnyEyes固定参数组 |
| 快速过流/过压阈值、确认拍数、软件保护总使能 | **未接入 DPRAM** | AnyEyes固定参数组 |
| 原软件 OC/OV 阈值和计时 | **未接入 DPRAM**，AnyEyes当前也无映射 | CCS/源码，或开启维护协议后用旧PARAMETER索引6..11 |
| 新预充阈值、预充稳定/超时、旁路延时、断路器脉冲 | **未接入 DPRAM** | AnyEyes固定参数组 |
| `HA_H_Voltage`、`HA_L_Voltage_Precharge` | **未接入 DPRAM、AnyEyes或维护协议** | CCS运行变量或源码默认值 |
| DAB起始/目标占空比及斜坡 | **未接入 DPRAM** | AnyEyes固定参数组 |
| 旧 `PORT_V_SET` | **已接入旧缓存/旧FPGA寄存器**，不改变新核心 DC 给定 | 若要改新闭环目标，应写 AnyEyes `group=0,index=6` |
| 旧 `STATCOM_Q_SET` | **已接入旧缓存/旧FPGA寄存器**，不改变新核心 Iq 给定 | 若要改新 Iq，应写 AnyEyes `group=0,index=7` |

AnyEyes 的现代参数 group/index 不是 DPRAM 地址。例如预充阈值为 `group=1,index=2`，预充稳定/超时/旁路/断路器时序为 `group=1,index=3..6`，PLL PI为 `group=1,index=8..9`，电压 PI为 `group=1,index=15..16`，电流 PI为 `group=1,index=24..25`，快速保护为 `group=1,index=30..32`。这些地址属于 AnyEyes 参数协议，不能直接写入 `0x6600xxxx`。VOFA本身只有JustFloat观测发送，不提供这些参数读写；选VOFA时可另开启旧维护协议，但旧维护参数不自动等于现代AnyEyes参数表。

`SST_ALLOW_SERIAL_COMMANDS=(!SST_SIMULATOR_MODE && !SST_ENABLE_DPRAM)`，因此启用DPRAM并允许远控时，AnyEyes普通启停/DAB/复位被拒绝，停机参数读写和显示仍可用；AnyEyes `FC` 急停独立有效。若改由AnyEyes普通遥控，应设置 `SST_ENABLE_DPRAM=0,SST_SIMULATOR_MODE=0`。

预充和保护阈值相互独立，当前均无DPRAM调参字段：现代 `precharge_voltage_v` 默认350 V；旧 `HA_L_Voltage_Precharge` 默认350 V（模块预充巡检）；旧 `HA_H_Voltage` 默认780 V（单模块高压状态巡检）；原软件 `OV_level_SET` 默认830 V（三相均压合成滤波值）；新快速 `dc_overvoltage_trip_v` 默认780 V（三相模块均压最大值）。`PORT_V_SET`是旧运行电压参考，不是预充或过压阈值。新旧PI字段名称相近也不代表访问的是同一个对象。

## 源码依据

本次文档核验记录见 [dpram_mapping_audit.json](evidence/dpram_mapping_audit.json)。核验内容为TI C6000 8.3.12编译器对4种线上结构的 `sizeof/offsetof` 编译期断言、CSV地址转换/唯一性、当前链接脚本及已有DPRAM映射文件。没有重新编译完整固件，没有进行硬件通信测试，未改变10 kHz FPGA中断路径。

- 固定地址和链接段：[`C6748.cmd`](../C6748.cmd#L94)、[`DpramRegister.c`](../Source/02_Peripheral/01_DPRAM/DpramRegister.c#L1)
- 块复制和邮箱协议：[`DPRAM.C`](../Source/02_Peripheral/01_DPRAM/DPRAM.C#L173)、[`DPRAM.C`](../Source/02_Peripheral/01_DPRAM/DPRAM.C#L338)
- DPRAM 中断和后台服务：[`Redef_Interrupt.c`](../Source/03_System/02_ISR/Redef_Interrupt.c#L598)、[`SstDpramHost.c`](../Source/04_App/10_Host/SstDpramHost.c#L21)
- 遥控权限和命令优先级：[`SstCommand.c`](../Source/04_App/04_Control/SstCommand.c#L90)
- 旧遥调实际消费：[`DPRAM.C`](../Source/02_Peripheral/01_DPRAM/DPRAM.C#L1376)
- 公共状态、遥测和旧故障位图：[`DPRAM.C`](../Source/02_Peripheral/01_DPRAM/DPRAM.C#L2903)、[`FaultCheck.C`](../Source/04_App/08_Fault/FaultCheck.C#L254)、[`SstPortStatus.c`](../Source/04_App/04_Control/SstPortStatus.c#L146)
- 现代 AnyEyes 参数映射：[`SstParameterTable.c`](../Source/04_App/01_FuncCode/SstParameterTable.c#L17)
