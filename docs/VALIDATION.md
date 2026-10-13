# SST 框架重构验证报告

## 当前并口维护调整（2026-10-13）

当前源码已删除485的MaintenanceProtocol/A5帧及使能宏，485仅选择AnyEyes或VOFA；默认改为DPRAM开启、仿真器关闭。旧固定区保留，新增54点维护、现代遥测/故障历史及16字后台分片，参数持久化改为页6/7的v2镜像108字节/50字并兼容v1。说明见 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)，本轮结果以 [dpram_maintenance_validation.json](evidence/dpram_maintenance_validation.json) 的输入、返回码和固件记录为准。

| 本轮检查 | 已完成结果 |
|---|---|
| TI目标Debug/Release | 各87个源文件编译、链接通过；各17个既有告警，无新增维护模块告警 |
| 宏配置矩阵 | AnyEyes/VOFA、仿真器、DPRAM的8种组合及2种上电恢复组合，共10种通过 |
| CCS导入及构建 | 独立CCS工作区导入、Debug/Release通过；项目位于工作区直接子目录的导入修正也通过 |
| 主机软件回归 | 13项通过，含串口范围、v1/v2存储兼容、并口CRC/序号/互锁/分片发布 |
| 独立点表核验 | 3038字地址唯一，原2142字布局不变，54点与真实参数表一致，TI类型大小核验通过 |
| 链接布局审计 | 14份map固定并口地址、NOINIT、无启动加载镜像通过 |
| 中断编译对照 | FPGA/FAULT/Timer2/DPRAM四个ISR的指令、直接调用、代码尺寸和栈量与固定参照一致；FPGAIsr为572字节、栈8字节 |
| 最终清单 | 14份固件产物及报告匹配源码输入SHA256 `13b5009537ca27f8b02ade3123f0e333594e8f7dc7fd18f2ace94b6419396b15` |

新增独立点表审计不重新生成输入CSV，避免生成器与审计共用同一错误。复验运行 `python tests/dpram_point_table_audit.py`，再生成清单与摘要。以上为软件检查；未执行实板并口握手、负载下的100µs时限或PWM测试。

以下2026-10-12及更早表格是固定历史版本的验证记录，包括已删除的485维护和旧默认宏配置；不能用其中的文件数、宏组合数、告警数或固件哈希认定本轮源码已通过。当前构建与回归结果由本轮证据记录，不在本文沿用历史数字。

历史保护注释修正：计时单位、回差和测量口径说明已更新，检查记录见 [protection_comment_correction.json](evidence/protection_comment_correction.json)。该注释阶段执行token和预处理指令相对 `framework-refactored-2026-10-12` 保持一致，当时未重新编译。下列完整构建证据及其字节哈希对应该固定标签。

## 历史框架交付验证（2026-10-12）

本节对应2026-10-12框架发布源码。固定参照为Git标签 `baseline-pre-framework-2026-10-12`、提交 `ddaeb56f21a81a70cdb76085728f91b67dc3c7a0`。模块对应、数据所有权、初始化步骤和调用关系见 [FRAMEWORK_REFACTOR_RESULT.md](FRAMEWORK_REFACTOR_RESULT.md)。该阶段结果、源码指纹、报告及固件哈希见 [software_validation.json](evidence/software_validation.json)；本机同名build文件可能已由新检查覆盖，应按指纹区分。

| 验证项 | 最终结果 | 本机证据 |
|---|---|---|
| TI Debug / Release | 每配置85源文件全部编译及链接通过 | `build/Debug/default/`、`build/Release/default/` |
| 11种宏组合 | 11/11通过 | `build/matrix_result.json` |
| CCS真实导入、Clean Build | 导入和两个配置通过；clean与build分别执行，均退出0 | `build/ccs_validation/result.json` |
| 工作区根目录下直接导入 | 复现旧名称冲突；AnyEyesVersion名称导入及两个配置构建通过 | `build/workspace_import_validation/result.json` |
| 主机验证套件 | 11/11通过 | `build/host_validation/result.json`及各检查日志 |
| 控制对照 / 实时静态审计 | 8场景、每版17,325快照一致 / 34项断言通过 | `control_regression.log`、`realtime_audit.log` |
| 协议、维护及DPRAM命令 | 各4配置通过；单字段写入、命令权限、STOP/OFF和保存互锁保持 | `protocol_regression.log`、`maintenance_regression.log`、`dpram_commands.log` |
| 故障报告 / 参数精度 | 编码、等级、首故障、历史及复位通过 / 37参数与5持久化命令字段通过 | `fault_reporting.log`、`parameter_precision.log` |
| 源码执行内容及归属 | 2,419原实体、2,324当前实体核验通过；22项精确审阅变更 | `build/refactor_equivalence.json` |
| 初始化分段 | 原3,120/175执行token及预处理指令顺序严格相同 | `build/control_init_equivalence.json` |
| 模块边界及公开头 | 11项结构断言、136个TI C/C++独立编译单元通过 | `build/framework_structure/result.json` |
| 自定义中文注释 | 124个自定义源文件/头文件覆盖通过 | `build/comment_inventory/verification.json` |
| 真实启动/后台/ISR轨迹 | 3种配置各17个场景通过，硬件和业务调用边界使用替身 | `build/firmware_trace/result.json` |
| ISR目标编译对照 | 4/4指令流、直接调用、栈帧、ELF函数尺寸一致 | `build/isr_binary_audit/result.json` |
| 硬件map及初始化表 | 15份map通过，FPGA/FT3/DPRAM原地址和布局保持；DPRAM零加载字节 | `build/dpram_map_validation.json` |
| 最终交付清单 | 所有源码/报告指纹一致，记录15份固件SHA256 | `build/delivery_manifest.json` |

上表主机日志完整路径为 `build/host_validation/<检查名>.log`。默认配置保留17条原告警，未启用维护的VOFA配置有18条；没有隐式函数声明 `warning #225`。两个目标配置保持原关闭优化策略。主程序RAM起点 `0x11801000`、长度 `0x3F000` 不变，最终用量/剩余量见摘要 `memory_usage`。普通RAM符号及函数地址随文件拆分重新链接，仿真器使用符号名。

源审计按函数、对象、类型、宏、条件上下文及硬件section比较执行token。原主文件516条对象归属11个数据域，类型、初值、条件保持；129个非活动历史实体在Source外归档。22项接口/组织调整逐项记录精确旧新哈希和回归证据，ISR、TAD常量、硬件section不能由一般合同豁免。没有相邻原SST目录的新克隆通过 `tests/git_baseline.py` 从固定标签提取控制/实时参照。

ISR审计使用相同TI Debug选项，只规范化编译器局部标签及排除调试指令，保留操作数和调用符号。FPGA/FAULT/Timer2/DPRAM的函数尺寸分别为572/132/1,104/104字节，各自局部栈帧均8字节，与基线一致。已链接代码哈希可因目标地址重定位而不同，不声称整个固件二进制相同。

重现最终验证（Python需 `tree-sitter`、`tree-sitter-c`、`tree-sitter-cpp`，另需TI/CCS和GCC）：

```powershell
python tools/build.py --configuration Debug
python tools/build.py --configuration Release
python tools/run_checks.py
python tools/build_matrix.py
python tools/ccs_validate.py
python tools/verify_workspace_import.py
python tests/isr_binary_audit.py
python tests/dpram_map_audit.py --delivery --report build/dpram_map_validation.json
python tools/delivery_manifest.py
python tools/export_validation_summary.py
```

交付清单拒绝过期结果：各构建、测试报告、源码和固件哈希必须对应当前文件。较大的本机日志及固件不进入Git，提交携带轻量摘要。详细审计方法见 [REFACTOR_VALIDATION_TOOLS.md](REFACTOR_VALIDATION_TOOLS.md)。

软件保持不代替物理100 μs测量。FPGA外部中断、原ISR快链及FPGA状态回写保留，通信/存储/历史报告在后台；原TI入口不主动开启中断嵌套。缓存、重新链接位置、EMIFA等待仍可能影响执行时间。本次未连接板卡；完整边沿到返回预算、漏边沿、PWM/保护、RS485、AnyEyes客户端及EEPROM掉电保存按 [BOARD_ACCEPTANCE.md](BOARD_ACCEPTANCE.md) 实测。下面2026-10-10记录仅为历史首版迁移证据，不对应重构后的文件数、RAM用量或当前固件。

## 历史首版迁移验证（2026-10-10）

工程使用TI C6000 8.3.12编译全部52个源文件，并以实际CCS 12.5导入构建验证。下列记录对应最终源码；编译输出、映射表和结果JSON随工程保存。

| 项目 | 结果 | 证据 |
|---|---|---|
| CCS导入、Debug、Release | 均退出0，两个配置均0项目错误 | `build/ccs_validation/result.json`及三份日志 |
| TI原生命令行Debug/Release | 全部源文件编译成功，链接退出0 | `build/Debug/default/`、`build/Release/default/` |
| 11项宏组合 | 11/11编译及链接通过 | `build/matrix_result.json`及各变体目录 |
| 控制算法对照 | 8场景、每版17,325个快照完全一致 | `build/host_validation/control_regression.log` |
| 实时路径静态审计 | 34/34通过；核心采样/控制/保护、ISR顺序、无新增ISR通信、DPRAM关闭路径及链接布局检查通过 | `build/host_validation/realtime_audit.log` |
| 链接表DPRAM审计 | 15/15通过；原地址、零加载字节、无启动初始化镜像 | `build/dpram_map_validation.json` |
| 故障报告 | 编码、等级、首故障、历史、去重及复位回归通过 | `build/host_validation/fault_reporting.log` |
| AnyEyes/VOFA | 4配置O2主机回归通过，帧、CRC、权限、恢复、串口预算、保存及心跳字节核对通过 | `build/host_validation/protocol_regression.log` |
| DPRAM控制 | 4种命令归属配置通过，STOP/OFF与保留位、存储互锁通过 | `build/host_validation/dpram_commands.log` |
| 可选维护协议 | 4配置通过，真实CRC16帧、停止优先、手动开关拒绝、两套保存互锁及页隔离通过 | `build/host_validation/maintenance_regression.log` |
| 参数精度 | 37参数+5个保存命令可表示，保存/恢复及重复加载稳定 | `build/host_validation/parameter_precision.log` |
| 自定义源码注释 | 46文件覆盖检查通过；补注释前后的代码标记及预处理指令46/46一致 | `build/comment_inventory/verification.json`、`docs/SOURCE_COMMENTS.md` |
| 固件校验清单 | 记录源码输入、15个固件产物的SHA256和成功结果 | `build/delivery_manifest.json` |

宏矩阵包含AnyEyes/VOFA × 仿真器/远程 × DPRAM关闭/开启的8种组合，另有AnyEyes维护、VOFA维护、AnyEyes上电参数加载3种组合。Debug与Release均沿用原关闭优化的策略；主机O2只用于回归，不改变TI目标编译选项。

最终CCS固件为 `Debug/SST_AnyEyes.out`、`Release/SST_AnyEyes.out`。默认AnyEyes+仿真器+DPRAM关闭配置有17条保留告警：9条TI驱动头文件续行空白告警，8条原DPRAM/ADC/FaultCheck未使用变量告警。没有屏蔽告警，也没有把“零错误”描述为“零告警”；新通信和命令适配未产生这些告警。4种未启用维护的VOFA配置另有1条`muxExpected`在该编译分支未被使用的告警（总18条），不影响链接；所有变体的告警数均保存在矩阵JSON。

主程序RAM区 `SHDSPL2RAM` 范围保持 `0x11801000`、长度 `0x3F000`；默认Debug/Release用量`0x262D6`、剩余`0x18D2A`，各变体以自己的 `.map` 为准。19个DPRAM输出section保持CS5原地址，增加`NOINIT`属性，取消运行库在EMIFA初始化前对外部通信区的清零。实际链接保留18段，未使用的Unit17被删除。关闭配置不注册DPRAM中断、后台不读写映射区；FT3故障字从本地`Port_RealFault`取得。开启配置保留原FT3来源与后台收发，新宏关闭语义可由静态调用检查和链接初始化表复核。

### 参数表示与量化

AnyEyes的16位参数视图采用比例整数。直接仿真器调试和默认加载关闭时，原float控制值保持原样；显式保存/恢复后使用映射的量化值。主机测得默认值中4项变化如下，均在半个编码单位内，重复加载不累积误差。

| 参数 | 原float | 保存恢复后 | 误差上限（半LSB） |
|---|---:|---:|---:|
| `pll_correction_limit_rad_s` | 62.831852 | 62.8300018 | 0.005 |
| `voltage_kp` | 0.0993750021 | 0.0993800014 | 0.000005 |
| `current_kp` | -1.24916661 | -1.24899995 | 0.0005 |
| `current_ki` | -16.666666 | -16.6669998 | 0.0005 |

保存不包含run/dab/fault_reset，也不包含软件保护开关；上电自动加载默认关闭。闭环参数量化后的实际控制品质需按板级验收核对。源代码对照回归验证的是原默认控制值下的行为等价，不把量化后的float值声称为逐位相同。

### 重现所有软件验证

```powershell
python tools/run_checks.py
python tools/build_matrix.py
python tools/build.py --configuration Debug
python tools/build.py --configuration Release
python tools/ccs_validate.py
python tests/dpram_map_audit.py --delivery --report build/dpram_map_validation.json
python tools/delivery_manifest.py
```

`run_checks.py`保存7类检查日志，并确认检查期间固件源码/工程配置未变化。`delivery_manifest.py`核对成功结果及当前源码指纹，记录实际固件文件。板级10kHz、物理PWM与保护响应、AnyEyes客户端及RS485电气、EEPROM掉电保存尚未实测，验收步骤见 [BOARD_ACCEPTANCE.md](BOARD_ACCEPTANCE.md)。

## Scope and current evidence

The baseline is `D:/Code/SST/Chb_DabPrim_vofa`. This project's control mathematics,
FPGA mappings, sampling and existing fast protection are retained. DPRAM-off FT3
reporting reads the local fault word rather than disabled external CS5 memory;
the linker marks only DPRAM sections NOINIT without changing their addresses. Validation
separates source equivalence, host behavioral regression, target compilation and
physical real-time measurement. Passing the first three does not prove the
physical 10 kHz deadline.

Validation on 2026-10-10:

- `tests/realtime_audit.py` passed its source invariants: control/sampling/
  protection files and the legacy shared header have identical executable tokens
  to the baseline (Chinese documentation comments were added);
  PluginUnit preserves enabled-DPRAM source tokens and only changes the FT3 fault
  source for disabled DPRAM. The FPGA ISR token sequence
  differs only by removal of the old VOFA scheduling flag; the hardware fault ISR
  is unchanged; GPIO69 bank4 still maps to INT5; hardware fault bank8 maps to INT4.
- New communication, parameter handling and fault reporting have no reachable
  call from FPGA, hardware fault, Timer2 or DPRAM ISRs. Their background paths do
  not call global interrupt-mask or cache-management APIs. Legacy UART interrupt
  setup remains as unused source and is not reachable from `main`.
- Control, fault and debug objects are initialized before enabling TAD and fault
  GPIO interrupt sources.
- `tests/control_regression.py` passed 8 scenarios, with 17,325 snapshots in each
  implementation. Every scenario's migrated trace exactly matches the original.
  Combined trace SHA256:
  `c34cd06dda3d78a2ae07e3fc333a5bd67e614b0613b6ccae31967b2eb2c50a40`.
- Target compile/link evidence is generated by `tools/build.py` in each build
  variant's `build_result.json`, `build.log` and `.map`. Check the relevant variant
  rather than assuming one configuration validates every feature combination.

## Reproduce the automated checks

From the project root, using Python 3 and a host GCC:

```powershell
python tests/realtime_audit.py
python tests/control_regression.py
```

Both scripts accept `--baseline` and `--project` overrides. The behavioral test
also accepts `--cc C:/path/to/gcc.exe`. It creates disposable translation units,
removes only local `#include` directives from each actual control source, and
compiles it with identical fake board definitions. It does not edit or link test
code into the target firmware.

The static check compares the complete source control files and complete TAD
body, not only named control calls. The supplemental call graph conservatively
examines available C function bodies; it does not model every preprocessor
configuration, indirect function pointer or target compiler intrinsic. Build
variant checks and manual review remain necessary.

| Host scenario | Assertions beyond original/migrated comparison |
|---|---|
| Open loop and stop priority | CHB/DAB run, DAB ramps 350 to 2500, absent slots remain untouched, open loop does not operate contactors, simultaneous stop/start chooses stop |
| DAB-only and independent gates | CHB neutral reference during DAB-only operation; CHB can start separately and DAB can stop without stopping CHB |
| Closed loop / compatible startup | Precharge, bypass and PLL qualification reach RUNNING; PLL settles near 50 Hz; voltage loop is active |
| Closed loop / Id ramp | Ramp phase starts with outer loop off and reaches voltage-loop handover/normal operation |
| Overcurrent, first fault and reset | Four samples do not trip; fifth trips; PWM gates close; later hardware fault does not overwrite first code; stopped reset returns to STOPPED |
| Overvoltage | Four samples do not trip; fifth trips and closes both PWM gates |
| Hardware fault | Hardware fault closes PWM even when the added software protection switch is disabled |
| Precharge timeout | Low DC voltage reaches timeout and remains with both PWM gates disabled |

At every snapshot the test hashes all public status fields, command enables,
FPGA reference/duty/start values and fake hardware actions. It asserts finite
floating-point status values. This checks behavior under deterministic sampled
inputs; it is not a plant simulation, hardware register-layout test, TI ABI test,
concurrency proof, full protocol integration test or real-time benchmark.

## Real-time constraints retained from the source

1. TAD is an external FPGA sampling edge on GPIO69, bank4, CPU INT5. Setting
   `sample_period_s = 0.0001` only configures the algorithm's integration interval;
   it does not establish the physical interrupt frequency. The FPGA must actually
   provide one edge per 100 us.
2. `FPGAIsr` disables GPIO bank4 while it processes one edge, clears the pending
   event, reads samples, computes control/protection and updates the DAC, then
   re-enables bank4. A late/long invocation can merge or lose subsequent events.
   GPIO43/45 high pulses bracket most of this work but omit the ISR prologue,
   initial bank operations and tail after the low write.
3. The TI `interrupt` wrappers in `00_Device/system_config/c674x/interrupt.c`
   dispatch the registered INT4/5/6/7/8 handler without explicitly enabling nested
   interrupts. C6748 maskable interrupt entry clears GIE until return. A higher
   priority INT5 therefore does not automatically preempt an already executing
   Timer2 handler. Timer2 still contains the source's counters and fault-latch
   reset path. The hardware fault handler also retains source work and runs at
   INT4. These paths need measured worst-case execution times.
4. Removing UART interrupt registration avoids introducing receive/parse work
   into that non-preemptible ISR context. UART polling, bounded RX/TX work,
   protocol parsing, queue handling and fault-history collection run in the
   cooperative background. GPIO/TIMER/UART peripheral masking is different from
   globally disabling CPU interrupts.
5. Control code uses `sinf`, `cosf`, `sqrtf` and EMIFA register accesses. Toolchain
   version, math library, compiler options, link placement, cache state, memory
   arbitration and EMIFA wait states all affect duration despite source equality.
   The target build intentionally retains the baseline's optimization policy;
   compile success is not an execution-time measurement.
6. `Chb_WrapAngle` has input-dependent loops retained from the source. Serial
   parameter bounds and the read-only sampling interval limit normal inputs, but
   arbitrary debugger writes can still create invalid/extreme values. The host
   regression covers valid operating inputs and does not certify arbitrary raw
   debugger mutation.
7. Background EEPROM loading can execute multiple bit-banged transfers. Saving
   and readback execute at most one byte operation per 10 ms tick. Storage runs
   only while stopped and leaves CPU interrupts enabled, so it is not
   placed in TAD. It can nevertheless delay background protocol servicing and
   overflow UART FIFOs under sustained input. Confirm storage operations and
   queue recovery on the actual board.
8. Fault reporting samples scalar software latches without disabling interrupts.
   Its record is a background observation, not a simultaneous hardware snapshot
   or timestamp of the original asynchronous fault edge. Immediate PWM closure
   remains in the source protection path.

## Required board measurements

Use a logic analyzer or oscilloscope with the TAD input (GPIO69), existing
GPIO43/45 probes, and actual FPGA/UNIT PWM outputs. If board probing permits,
capture the fault input too. Keep the source firmware and migrated firmware's
toolchain/build mode comparable.

| Measurement | Acceptance evidence |
|---|---|
| TAD source period | Nominal 100 us and bounded jitter, measured externally |
| Input edge to GPIO43 rise | Worst-case dispatch latency, including Timer2/fault contention |
| GPIO43 high duration | Control-body duration for stopped, open loop, PLL acquisition, closed loop, ramp and fault branches |
| Full edge-to-return budget | Entry latency + full ISR time remains below 100 us with measured margin; add prologue/tail measurement or inspect cycle counts, do not use the body probe alone |
| Missed edges | Compare edge count against completed ISR pulses over a long capture; no unexplained gap or merged TAD event |
| PWM result | Three-phase reference order, amplitude, frequency, CHB/DAB enable independence, 350-to-2500 duty ramp and configured absent modules match the baseline |
| Protection latency | External hardware fault to physical PWM closure, including a fault arriving during TAD; software OC/OV confirmation operates at the intended sample count |
| Communication stress | Repeated valid commands, bad CRC, short/oversized frames, sustained RX, maximum scope channels and TX backpressure do not violate the TAD budget |
| Stopped storage | Save/load and failed storage read/write recover cleanly without inadvertent run or FIFO deadlock |

Record min/max latency, body time, full ISR cycles, observed edge count, firmware
hash, build configuration, clock, cache/EMIFA configuration and test duration.
Physical timing and PWM tests have not been performed by the host-only checks.
