# UNIT 逐路在线确认和回传超时保护

2026-11-22

## 基线

改动前先将当前 DSP 工作区与 501、601、701 和 UNIT FanBalance 的设计文件发布到 GitHub。提交为 `cd9368f9501c4504a5e94894a5ebcc1b20e8d2cd`，分支为 `snapshot/before-unit-heartbeat-20261122`。基线附 `.idf` 与生成说明，Pango 标明限制公开分发的生成 RTL 仍在本机。GitHub 副本将 Windows 保留文件名 `aux.v` 改用 `dual_led.v` 并同步 PDS 引用。 [GitHub 基线](https://github.com/Rojya66/SST-AnyEyes/commit/cd9368f9501c4504a5e94894a5ebcc1b20e8d2cd)

## 修改前后

| 环节 | 修改前 | 修改后 |
| --- | --- | --- |
| 501 回传 | Udc 寄存器保存最后一次数据，串口失联后读回仍可能是原电压。 | 每路 Udc 收帧刷新在线计时。F1 分别报告链路在线和电压可用，F2 保留超时位置。 |
| 闭环预充 | 用配置相的平均母线电压连续达标推进，未逐路核对帧。 | QF1 合位后逐路核对当前 PM，且收到真实 Udc 更新后才布防 701。全部所需回传、电压和已启用 FO/FB 都正常，才累计原预充连续时间。 |
| 旁路、PLL、发波 | 按原开关反馈、母线条件、锁相判断。 | 旁路确认、等 PLL 和发波门控继续使用在线状态。预充和等 PLL 期间，已恢复的短超时会清连续计数。 |
| 闭环运行 | 701 可报告独立故障光纤失光。仅 501 回传断开时，仍可能保留旧值。 | 持续失联或 F2 锁存超时走现有故障停机流程，归类通信故障 `39103`，保存各相物理通道。701 故障位置沿用原 FO 锁存和 DPRAM 显示。 |
| 701 和启停 | DSP 初始化后应用有效 FO/FB，QF1 合闸后才取电的 UNIT 可能在启动前已锁存失光。 | 开机延后 701 布防。闭环在 UNIT 回传满足后布防，正常分 QF1 前撤防。故障锁存保留至原有明确复位。开环在首次发波前应用其 FO 配置，发波仍走原开环路径。 |

## 501 寄存器

`0xF1` 低字节为最近收到 Udc 帧的物理口位图。UART 接收器检查停止位，501 检查 Udc 标签 `000`。帧内容等于上一次时正常续期。100 MHz 下计时 `100000` 个时钟，即 1 ms，名义上约 20 个 UNIT Udc 周期。`0xF1` 高字节是已写入电压缓存且可用于当前判断的口。BK 冻结只关闭电压可用性，解冻之后需新收并提交一帧，冻结期间暂存的帧无法恢复高位。

`0xF2` 低字节锁存已在线通道的超时，回传恢复不会清除，按位写 1 清除。清除与超时同拍时优先置位。撤销 PM 或执行原有 `LED=0x0538` 清缓存时清该路状态。DSP 按当前 PM 消费所需通道，CCS 可读 F2 观察历史事件，读操作不清锁存。查短失联时可看软件状态，因为 DSP 已消费的 F2 位会回到 0。

| 501 槽位 | `UNIT_LINK` / F1 | `UNIT_LINK_LOSS` / F2 |
| --- | --- | --- |
| A1 / U5 | `0x600007E2` | `0x600007E4` |
| A2 / U6 | `0x600009E2` | `0x600009E4` |
| B1 / U9 | `0x60000FE2` | `0x60000FE4` |
| B2 / U10 | `0x600011E2` | `0x600011E4` |
| C1 / U13 | `0x600017E2` | `0x600017E4` |
| C2 / U14 | `0x600019E2` | `0x600019E4` |

每槽仍为 0x200 字节。新字段占用原 `Reserved_8[0..1]`。例如只配置 A1 第 1 口，则 A1 通信和电压都有效时 F1 为 `0x0101`，断回传超过 1 ms 时为 `0x0000`，F2 第 0 位锁存。A2 第一口对应整相 UNIT 9，诊断位图是 `0x0100`。

## DSP 判据与查看

预充沿用原 10 ms 节拍、200 ms 连续达标与 10 s 默认超时。比较的是配置相原有平均母线电压，附加每路数据可用条件。首次写入 701 布防的那拍不计入放行，后续检查 701 实时 FO、首锁存和 403 对应汇总后再累计。既有 202 采样保护继续参与 403。701 复位仍沿用现有路径，预充不会自动清全局故障锁存。

CCS 查看 `g_chb_dab_status.unit_link_missing[0..2]`，A/B/C 各 16 位，bit0 对应物理 UNIT 1，bit8 对应 UNIT 9。上位机通信故障查原故障码 `39103`，701 第一故障位置仍查 `051E/051F/0520`。两类数据分别保留。 现有 501 插件原始镜像也会带出这两个字，例如 A1 的屏幕字地址为 `09F1/09F2`。镜像按原周期刷新，实时排查以 CCS 直接读和 DSP 的失联记录为准。

与板上时序有关的边界包括 1 ms 收帧超时、DSP 下一拍检查的时间以及 QF1 后 UNIT 上电加载时间。小于 1 ms 且未中断 Udc 超时计时的重启仍可能不触发这条检查。Udc 心跳验证上行回传。若需要核对 501 下行命令被 UNIT 接收，则涉及独立的命令回执。实际 Flash 加载接近默认 10 s 预充超时时，应核对首帧到达后是否还有足够的连续达标时间。

烧录 501 和 DSP 时使用配套版本，旧 501 的保留地址可能回显上一次总线读值。可在主功率闭锁且有辅助供电的台架上查看单路 F1 `0x0101`，仅断 501 回传观察其下降并置 F2，同时查看 701 FO 仍反映自身独立链路。恢复回传后 F1 恢复，再检查软件的失联位置与原故障复位流程。

[501 工程](D:/Code/FPGA/Pango/501_DSP_Compatible/501_DSP_Compatible.pds) · [501 超时仿真](D:/Code/FPGA/Pango/501_DSP_Compatible/tests/tb_unit_udc_watchdog.sv) · [DSP 工程](D:/Code/SST/AnyEyesVersion_DpramCompat/.project)

## 验证

501 Icarus 仿真通过。使用真实 UART 与 EMIFA、仿真 PLL 和乘法模型，覆盖同值重复、错误停止位、非 Udc 帧、各路独立、BK、恢复后的 F2、写 1 清零以及清零同拍超时。超时参数在仿真里缩短。FPGA 综合、烧录及实板联调留待现场。

| DSP 验证 | 结果 |
| --- | --- |
| `python tests/single_phase_control.py --unit-link-only` | 四组通过。覆盖预充所需位图、701 布防隔拍、F2 短失联、PLL 重新确认、运行失联停机、零时间参数、正常停机仲裁及首次开环 DAB。 |
| `python tests/switch_io.py` | 六种单双相和开关输出、反馈配置通过。含维护分闸撤防，以及已锁存 FO 的故障保留。 |
| `python tests/eeprom_module_layout.py` | 通过 256 组稀疏 PM，核对 F1、F2 的字节偏移及 FO/FB 布防。 |
| `python tests/fo_fault_telemetry.py` | 单相和三相均通过，原 `051E/051F/0520` 和 701 位置诊断正常。 |
| TI C6748 Debug | 默认 A 相和三相启用 EEPROM 恢复两组均编译、链接成功，每组 96 个源文件，各有 17 条原有警告。 |

全量 `single_phase_control`、`common_control_safety` 和 `bench_inputs` 的旧断言在 `cd9368f` 的生产源码上可复现。`control_regression` 引用的外部旧工程与当前测试桩存在 `Port_DOUT` 和 CHB 数据类型不兼容。上述全量测试未记为通过。

[DSP 在线检查](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/04_Control/ChbDabControl.c:861) · [FPGA 寄存器结构](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/02_Peripheral/00_FPGA/FpgaRegister.h:335) · [TI 默认构建](D:/Code/SST/AnyEyesVersion_DpramCompat/build/Debug/hvac-unitlink/build_result.json) · [TI 三相 EEPROM 构建](D:/Code/SST/AnyEyesVersion_DpramCompat/build/Debug/hvac-unitlink-three-phase/build_result.json)

