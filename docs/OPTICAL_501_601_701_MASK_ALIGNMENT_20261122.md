# 501、601、701 光纤极性与 DSP 掩码修改记录

2026-11-22

501 新工程从 `501_compatible_build` 复制设计源文件、管脚约束和所需 IP，工程名为 `501_DSP_Compatible`。601、701 和 DSP 在各自现有工程中修改。 [新 501 工程](D:/Code/FPGA/Pango/501_DSP_Compatible/501_DSP_Compatible.pds) · [601 工程](D:/Code/FPGA/Pango/601_openlooptest/601_openlooptest.pds) · [701 工程](D:/Code/FPGA/Pango/701/701.pds)

## 1. 原理图和光纤电平

| 板卡 | 图纸页 | 通道及电平 |
| --- | --- | --- |
| 501 | 13、14 | TX1–8 为 AFBR-1629Z，电气输入 0 发光、1 熄光。RX1–8 为 AFBR-2529Z，亮光输出 0，无光输出 1。UART 空闲为 1。 |
| 601 | 13、14 | 两页分别为 TX1–8、TX9–16，均为 AFBR-1629Z。每个发射口的电气输入 1 表示关断该模块。 |
| 701 | 13、14 | 两页分别为 RX1–8、RX9–16，均为 AFBR-2529Z。`Data_OUT` 经 33 Ω 串联电阻进入 FPGA `STATUS`，输出有 5.1 kΩ 上拉，链路无反相器。FPGA 采到 1 表示失光。 |

Broadcom `AV02-4369EN` 第 6 页 TX Data In 注 a 定义了 `AFBR-16x9Z` 的输出极性，第 11 页接收光学特性的 Off-State 注 b 定义了 `AFBR-25x9Z` 无光输出高。:codex-file-citation{path="D:/OneDrive/WorkFiles/SST/原理图/AV02-4369EN-AFBR-16xxZ-AFBR-26x4Z-25x9Z-DS.pdf" purpose="source"}

板级图纸。:codex-file-citation{path="D:/Code/Gemini/SST/NewPlatform/Schematic/sst501_v1_SCH.pdf" purpose="source"} :codex-file-citation{path="D:/Code/Gemini/SST/NewPlatform/Schematic/sst601_v1_SCH.pdf" purpose="source"} :codex-file-citation{path="D:/Code/Gemini/SST/NewPlatform/Schematic/sst701_v1_SCh.pdf" purpose="source"}

UNIT 图纸第 4 页 `OP_FO` 为 FPGA F19，第 10 页直接接故障发射器 HT2 的 `IN`，该网有 4.7 kΩ 下拉。HT2 图纸标注为 `HFBR-1629Z`，厂商资料型号为 `AFBR-1629Z`，实装料号待核实。以现有 `UnitBoard_DAB_Test_FanBalance` 的两个旁路常量均为 0 为条件，`OP_FO = fault_svg | fault_dab`。若装的是 AFBR-1629Z，健康时 `OP_FO=0` 并发光，故障时 `OP_FO=1` 并熄光，701 收到高电平。 [UNIT 逻辑](D:/Code/FPGA/Pango/UnitBoard_DAB_Test_FanBalance/source/main.v:674) :codex-file-citation{path="D:/Code/Gemini/SST/NewPlatform/Schematic/UNIT.pdf" purpose="source"}

## 2. 接线与掩码

整机 PDF 第 26–29 页对应图纸 33–36。每相第一张 501 的 1–8 口对应 UNIT 1–8，第二张 501 的 1–6 口对应 UNIT 9–14。601 和 701 的物理通道与 UNIT 同号，15、16 口在图纸中未接。历史现场保留 8 号位时，第二张 501 的 1 口仍对应 UNIT 9。 :codex-file-citation{path="D:/Applications/WeChat/xwechat_files/wzcrystal_c6a8/msg/file/2026-11/SST项目整机图纸(20261104).pdf" purpose="source"}

每张 501 取自己 `PM` 的低 8 位。将同一相的两张 501 拼成 `M = PM1 | (PM2 << 8)`，`M` 的 bit0 对应 UNIT 1，bit7 对应 UNIT 8，bit8 对应 UNIT 9。`NUM` 仍为该相两组置位总数。 [DSP 默认位图](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/04_Control/ChbDabControl.h:23) · [初始化与 NUM](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/02_Peripheral/00_FPGA/FpgaInit.c:87)

| 同相实际装配 | PM1 | PM2 | M | 601 `SHUTDOWN = ~M` | 701 `FO_EN`（原始配置为 `0x3FFF`） |
| --- | ---: | ---: | ---: | ---: | ---: |
| 仅 UNIT 1 | `0001` | `0000` | `0001` | `FFFE` | `0001` |
| 图纸 14 个模块 | `00FF` | `003F` | `3FFF` | `C000` | `3FFF` |
| 预留 UNIT 8，仍接 UNIT 9–14 | `007F` | `003F` | `3F7F` | `C080` | `3F7F` |
| 整相停用 | `0000` | `0000` | `0000` | `FFFF` | `0000` |

## 3. 修改前后

| 位置 | 修改前 | 修改后 |
| --- | --- | --- |
| 501 工程与 DAB 地址 | 兼容开环模式在 `0x30–0x38` 放三路独立参数，写该区可进入兼容模式，`0x39` 切换模式。带 Pango 插入式调试配置。 | 新工程删除该模式和插入式调试配置。`0x30–0x39` 读 0，写入忽略。常用 DAB 写入 `0x0A–0x0C`，逐路移相微调仍在 `0xE8–0xEF`，总线自检保留 `0x3A/0x3B`。 |
| 501 发射 | 常规周期帧仅用 `TX_EN`，兼容开环才叠加 `PM`。原始发送用 `TX_CTL`，可发到未配置口。 | 周期帧用 `TX_EN & PM`，原始发送用 `TX_CTL & PM`。`PM=0` 的 TX 输出 1，发射器熄光。 |
| 501 接收 | 各路 UART 持续解码。部分测量值在收到帧后再与 `PM` 相与，原始 RX 仍可更新。 | `PM=0` 时对应 UART 保持空闲，实时原始 RX、测量及状态清零。BK 滤波器未使用的 `RESET` 明确接 0。 |
| 601 发射和初始化 | 板上 `SHUTDOWN` 上电为 `FFFF`，旧 DSP 初始化覆盖为 `0000`，总关断释放后未选光口也可亮。 | 板上仍以 `FFFF` 上电，DSP 先写 `FFFF`，再写同相 `~M`。FPGA 将 16 路统一表达为 `OP_OUT = reg_SHUTDOWN \| {16{shut}}`，滤波器 `RESET` 接 0。 |
| 701 接收与初始化 | `FO_EN` 上电 `FFFF`。`~OP_IN` 送滤波，使用新接收器时亮光会被报成故障。 | `FO_EN` 上电 `0000`。`OP_IN` 直接滤波，DSP 下发 `Port_FO_x_EN & M`。所有未使用的滤波 `RESET` 接 0，`HW_RST` 仍走原复位输入。 |
| 403 汇总及 DSP 中断 | DSP 将配置 `Port_FB_EN` 直接写入 403，GPIO142 配为输出。 | 三块 701 的汇总位只在该板有有效 FO 时参与 403 汇总。默认 A/B/C 汇总位为 bit6/10/14，采样板 U2 为 bit0。GPIO142 改为输入，接 403 的 `DSP_INT_FAULT`。 |
| DSP 启动和参数 | 链接镜像包含 FPGA MMIO 清零记录。原 FO 缺省值固定为 `3F7F`。 | FPGA MMIO 设 `NOINIT`。先闭锁输出，再写布局，恢复可选 EEPROM 后应用有效 FO/FB、发起锁存复位并使能中断。原始 FO 缺省值为 `3FFF`，EEPROM、DPRAM 保存及回读的仍是用户配置。 |
| 模块故障位置 | DSP 读 701 锁存后仅按原始 FO 配置筛选。 | 软件故障分类按有效 FO/FB；屏幕按 `FO_LATCH & Port_FO_x_EN & M` 上报，位序按物理口。 |

[501 发送逻辑](D:/Code/FPGA/Pango/501_DSP_Compatible/source/main501.v:450) · [501 接收逻辑](D:/Code/FPGA/Pango/501_DSP_Compatible/source/main501.v:625) · [601 输出逻辑](D:/Code/FPGA/Pango/601_openlooptest/source/main601.v:167) · [701 初始化](D:/Code/FPGA/Pango/701/source/main701.v:78) · [DSP 掩码](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/02_Peripheral/00_FPGA/FpgaIo.c:622) · [DSP 初始化](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/03_System/01_OS/SystemInit.c:87) · [GPIO142](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/03_System/00_Board/SstBoard.c:150)

v3 EEPROM 恢复、DPRAM 原遥调及扩展参数写入都经过同一 FO/FB 硬件访问函数。若已有 EEPROM 保存 `3F7F`，显式关闭的 UNIT 8 继续关闭，即使当前 PM 包含 UNIT 8。启用该口需要 PM 和原始 FO 配置同时置位。 [EEPROM 硬件参数](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/01_FuncCode/SstHardwareParameters.c:87) · [DPRAM 应用](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/10_Host/SstDpramParameter.c:139) · [默认值](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/10_Host/DpramDefaults.c:24)

`NOINIT` 移除了 `main()` 前对 `0x60000000–0x60001DFF` 的 7680 字节清零记录，403 `DSP_CTL`、601 `SHUTDOWN` 与 501 `PM/TX_EN` 均在该范围。通过 TI 链接结果对比，其余四项 `.cinit` 记录的运行地址、长度和负载字节一致。 [链接配置](D:/Code/SST/AnyEyesVersion_DpramCompat/C6748.cmd:252) · [TI 链接 map](D:/Code/SST/AnyEyesVersion_DpramCompat/build/Debug/hvac-optical-layout-noinit/SST_DpramCompat.map)

## 4. EMIFA 字地址与 CCS 地址

FPGA `XA` 是 16 位字地址。CCS 中字节地址为 `0x60000000 + 0x200 × (槽号 - 2) + 2 × XA`。501 A1/A2、B1/B2、C1/C2 槽位依次为 U5/U6、U9/U10、U13/U14。 [DSP 结构体](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/02_Peripheral/00_FPGA/FpgaRegister.h:204) · [槽位定义](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/03_System/00_Board/SstBoardConfig.h)

| 寄存器 | XA | A 相 / 501 A1 | B 相 | C 相 |
| --- | --- | --- | --- | --- |
| 501 `NUM / PM / Reference` | `02 / 03 / 04` | `0x60000604 / 0x60000606 / 0x60000608` | | |
| 501 `DAB_LEFTDuty / DAB_RIGHTDuty / DAB_Start` | `0A–0C` | `0x60000614–0x60000618` | | |
| 501 `PM_CTL / TX_EN / TX_CTL` | `0D–0F` | `0x6000061A / 0x6000061C / 0x6000061E` | | |
| 501 `TX[0..7] / RX[0..7]` | `10–17 / 18–1F` | `0x60000620–0x6000062E / 0x60000630–0x6000063E` | | |
| 601 `SHUTDOWN` | `22` | `0x60000A44` | `0x60001244` | `0x60001A44` |
| 701 `FO_EN` | `23` | `0x60000C46` | `0x60001446` | `0x60001C46` |
| 701 `FO` 实时 | `E4` | `0x60000DC8` | `0x600015C8` | `0x60001DC8` |
| 701 `FO_LATCH` 首次锁存 | `E5` | `0x60000DCA` | `0x600015CA` | `0x60001DCA` |
| 403 `FB_EN / DSP_CTL`（三相共用） | `39 / 3F` | `0x60000472 / 0x6000047E` | | |

`0x6000047E.bit0` 从 1 变 0 表示总关断已释放。`0x60000A44` 是独立的 A 相 601 本地关断寄存器，启动命令只改变总关断时，该字可以保持常值。若旧现场读到 `0001`，bit0 为 1，TX1 仍熄灭。其余关断位为 0，总关断释放后其余口发光。仅凭这两个读数可以解释灯光，无法定位旧现场 `0001` 的写入来源。

当前源代码默认 A1=`0001`，其他五组为 0。未被有效 EEPROM 覆盖时，A1 501 `PM/TX_EN` 都是 `0001`，其他 501 无配置通道。601 A/B/C 读回 `FFFE/FFFF/FFFF`，701 A/B/C 的 `FO_EN` 读回 `0001/0000/0000`，403 `FB_EN` 从原始配置 `4441` 裁为 `0041`。总关断为 1 时 601 全灭，释放后只亮 A 相 TX1。

## 5. 701 定位和预充电

每块 701 的 `FO` 反映当前有效失光通道。`FO_LATCH` 保存该板首次触发时的位图，`FB` 同时锁存。后发故障出现在 `FO` 中，首次位图继续保留。故障消失且执行复位后才解除锁存。若 UNIT 9 先失光，`FO_LATCH=0100`。随后 UNIT 1 也失光，`FO=0101`，`FO_LATCH` 仍是 `0100`。A/B/C 相首故障分别发布至 DPRAM `051E/051F/0520`，DSP 侧字节地址 `0x66000A3C/0x66000A3E/0x66000A40`。 [701 锁存](D:/Code/FPGA/Pango/701/source/main701.v:302) · [DSP 故障分类](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/08_Fault/FaultCheck.C:454) · [屏幕遥测](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/10_Host/SstDpramTelemetry.c:100)

现有闭环预充推进依据配置相的平均母线电压连续达标，并结合已配置的开关反馈。`CHB_PM_*_MASK` 表示配置的物理通道，程序没有逐路实时在线握手来确认所有选中的 UNIT 得电并持续运行。已启用通道的 UNIT 掉电、故障或断纤都可导致 701 失光。原路径在 `Port_Stop=1/2` 时跳过 701 软件故障分类，403 发给 DSP 的信号使用上升沿。若某相在待机就已失光并保持锁存，进入预充时可能没有新上升沿。DPRAM 的位置遥测直接读 701，仍可看到已启用口的锁存。 [预充判据](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/04_Control/ChbDabControl.c:1637) · [待机故障处理](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/08_Fault/FaultCheck.C:528) · [上升沿设置](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/03_System/02_ISR/Redef_Interrupt.c:615)

## 6. 风扇与清华原版对照

原版 Xilinx UNIT 的 `LED_SVG/LED_DAB` 高有效，任一桥正常运行时跟随 `~FAN_OFF`，其余时间由本地 600/500 采样量滞回及 OV/UV 状态控制。原 DSP 的调试指令 `F3A0/F3A1` 分别写入停扇、开扇命令。 [原版 UNIT](D:/Code/Gemini/SST/11.监控系统软件设计说明/模块控制板源码与说明/模块控制板/Software_v6_AD7606/main.v:1021) · [原 DSP 风扇命令](D:/Code/Gemini/SST/端口控制器/主控插件DSP/HVAC_HVDC_LVDC/main.c:3899) · [预充和高压取电讨论](https://chatgpt.com/s/cx_6affa446eaa0819199aa6b48a3d88945)

当前 FanBalance 工程以 `svg_running/dab_running` 表示桥使能、无故障、无关断，外部 LED 分别取反。两桥均未健康运行时，当前源码以 `SCI_UDC > 235` 或 OV 为开扇条件，以 `SCI_UDC < 170` 为停扇条件。235 和 170 是代码比较值，实测门限取决于采样标定。运行任一桥时两个高有效输出 `FP_OUT0/FP_OUT1` 一起跟随 `~FAN_OFF`。DSP 在初始化和进预充时写 `FAN_OFF=0`，使运行分支的风扇命令为开启。当前应用未接入原版的独立停扇调试指令。本次校正了两处写成 600/500 的注释，采用当前 FanBalance 工程已有的比较值及转移条件。工程的 FIC 源列表为空，活动 RTL 未发现调试核标记。 [FanBalance 风扇代码](D:/Code/FPGA/Pango/UnitBoard_DAB_Test_FanBalance/source/main.v:1111) · [DSP 预充命令](D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/04_Control/ChbDabControl.c:1080)

## 7. 验证结果

| 检查 | 结果 |
| --- | --- |
| 501 Icarus 顶层仿真 | 通过。覆盖实际 EMIFA 地址、UART 帧、禁用收发、BK 锁存、旧兼容区域、DAB 参数和通道撤销重配。PLL 和乘法器用仿真替代模型。 |
| 601 Icarus 顶层仿真 | 通过。`XA=22` 写读、上电 `FFFF`、总关断、单路 `FFFE` 及稀疏 `FEFA` 的 TX 电平符合预期。PLL 用直通模型。 |
| 701 Icarus 顶层仿真 | 通过。配置前改变 RX，`FO/FO_LATCH/FB` 均为零。配置 `3F7F/3FFF` 后验证逐路位置、滤波、后发故障、首次锁存与复位。PLL 用直通模型。 |
| TI C6748 | 源码默认和启用 EEPROM 恢复两组独立 Debug 构建均成功，各编译 96 个源文件，各有 17 条警告。用 `offsetof` 核对 `0x200` 字节槽距、24 个寄存器字偏移和 13 个槽位基址。 |
| DSP 回归 | `eeprom_module_layout.py` 通过 256 组稀疏布局。`fo_fault_telemetry.py` 通过单相、三相和三个只读位图地址检查。`dpram_screen.py` 四组构建配置均通过。EEPROM 迁移、单相/三相 I/O、DPRAM 维护测试通过。 |
| 旧参数点表审计 | `dpram_point_table_audit.py` 在现有工作区的点 14 报错。CSV 仍为 `bypass_delay_10ms`，已有源码修改为 `kt1_settle_10ms`。该参数审计与本次的三个 701 位图检查分别执行。 |

当前验证基于原理图、RTL 仿真和 DSP 软件构建。FPGA 综合、烧录及实板光链路测量待完成。上板可按单模块配置依次核对 A 相 601 读回 `FFFE` 和 TX1 的亮灭，再在 701 断开对应光纤，检查 RX1 由低变高、`FO/FO_LATCH=0001` 和 `051E=0001`。复光、执行硬件故障复位后，检查锁存回到 0。若还需核对第二张 501 与 701 的位置映射，使用 UNIT 9 和位图 `0100`。

[TI 默认构建](D:/Code/SST/AnyEyesVersion_DpramCompat/build/Debug/hvac-optical-layout-source-default/build_result.json) · [TI EEPROM 构建](D:/Code/SST/AnyEyesVersion_DpramCompat/build/Debug/hvac-optical-layout-noinit/build_result.json) · [布局测试](D:/Code/SST/AnyEyesVersion_DpramCompat/tests/eeprom_module_layout.py) · [故障遥测测试](D:/Code/SST/AnyEyesVersion_DpramCompat/tests/fo_fault_telemetry.py)
