# 重构验证工具与等价证据

本次重构的原始参照为 Git 标签 `baseline-pre-framework-2026-10-12`，提交为 `ddaeb56f21a81a70cdb76085728f91b67dc3c7a0`。工具读取原始提交，不用整理后的实现覆盖参照。随版本保存的最终汇总为 [software_validation.json](evidence/software_validation.json)，完整本地产物清单为 `build/delivery_manifest.json`；报告中保留输入指纹、各项返回码和产物哈希。

## 源码搬运、接口变化与历史归档

`tests/refactor_equivalence.py` 通过 Tree-sitter 提取全自定义源中的完整函数、每个对象定义、公开声明、类型、宏和 `DATA_SECTION`。位置变化不影响等价判定；函数签名与正文、对象类型与初值、宏表达式和硬件段分配仍逐项进行词法 SHA256 比较。注释、换行和空白不影响比较。条件编译的外层上下文保留在报告中；固定 HVAC 分支与 C/C++ 链接包装按本工程既有选择处理，其他条件参与散列。

重复的公开声明允许收敛，函数及对象定义的数量不允许无声变化。未活动历史声明/函数只能由 `archive` 或 `Legacy` 中具有相同原始可执行 token 的文件满足，并明确记录为不参与编译的归档。归档不是当前运行实现等价的证明。活动入口及中断可达性还由实时审计、TI 构建和调用轨迹验证。

`tests/refactor_contracts.json` 记录经过审阅的后台接口/组织调整。每条列出原符号、替代符号、精确前后 hash、修改理由及回归证据。散列不匹配或无效的旧合同会失败。ISR 正文、`ControlTAD/TAD_PLL` 和硬件段映射禁止使用此类合同豁免。

`Initial_Variable/Variable_RST` 不是一般合同：审计直接调用 `tools/control_init_equivalence.py` 的 `verify()`，展开有序的无参域初始化函数，逐项核验迁移的局部类型，比较完整原始语句、预处理指令和次序。16 个上电段展开后，`Initial_Variable` 的 3,120 个执行 token、14 条预处理指令严格一致；`Variable_RST` 的 175 个执行 token、2 条预处理指令严格一致。工具允许原局部变量随所属段搬迁，但拒绝改变类型、增加初始化或在协调层增加额外执行语句。

结果为 `build/refactor_equivalence.json`，包含旧/新函数和对象位置、散列、归档位置、审阅说明及新增符号。

## 实时与结构约束

`tests/realtime_audit.py` 保留迁移时的 34 项断言，并改为全源定位，避免文件搬运导致假失败。其内容包括完整 TAD token 等价、唯一 `FastStep` 调用者、中断映射、DPRAM 关闭时不可达地址、初始化开放门槛、后台禁止全局关中断/CSR直接操作/Cache调用。函数调用图属于保守静态源码分析，不能代替目标二进制和板级测量。

`control_regression.py/realtime_audit.py` 优先使用相邻原 SST 工程；在新的 Git clone 中没有该目录时，用 `tests/git_baseline.py` 将标签中的必要文件读取到临时目录。输出明确标示使用外部原项目或重构前 Git 提交。临时旧 linker 参照仅移除已知 19 条 `NOINIT` 迁移属性以继续核验原迁移断言，标签及当前文件都不修改。

`tests/framework_structure.py` 核验以下结构：

- `Main` 只拥有程序入口，保留原启动顺序、20 s 等待和 pulse-test 条件分支。
- `TaskSlice` 保留心跳/控制/故障/DPRAM/通信/存储/测量顺序；调试节拍在首个 TX 前捕获。
- 活动代码不包含旧 `main.H/PluginUnit.H/Stateflow.H`。
- 每个外部函数、对象及 MMIO 段有唯一编译单元归属；公开头无包含环。
- 所有自定义公开头可使用 TI C6000 独立编译：`.h` 分别按 C、C++；`.hpp` 按 C++。
- 默认 TI Debug 日志没有隐式函数声明诊断 `warning #225`。

结构结果为 `build/framework_structure/result.json`，列出实际独立头编译单元及其诊断；文件 SHA256 用于交付时校验报告没有过期。

`tests/firmware_trace.py/.c` 分别编译真实 `SystemInit/TaskSlice/CpuTask/Redef_Interrupt` 和所属数据单元，仅替换 MMIO及外围/业务被调用函数。默认、DPRAM、EEPROM+维护 3 种配置各执行 17 个场景，核验 20 s Timer2时基、启动中断开放门槛、后台任务顺序、开闭环 TAD、硬件/软件故障分支、停机优先、无效边沿、FAULT/DPRAM/Timer2应答及单调时钟。结果为 `build/firmware_trace/result.json`；该运行轨迹与严格 ISR 正文等价、二进制审计、板级测量分别提供不同证据。

## ISR 的目标编译器对照

`tests/isr_binary_audit.py` 从固定 Git 标签建立临时基线工程，使用同一 TI C6000 8.3.12 Debug 参数编译旧入口和新的 `Redef_Interrupt.c`。比较编译器生成的符号汇编指令流，只规范化生成的局部标签并排除调试指令；操作码、操作数和直接调用符号参与比较。再读取生成的栈帧信息与链接 ELF 中四个函数的尺寸。

| 函数 | 基线/当前 ELF 尺寸（字节） | 基线/当前栈帧（字节） | 指令流及直接调用顺序 |
|---|---:|---:|---|
| `FPGAIsr` | 572 / 572 | 8 / 8 | 一致 |
| `FAULTIsr` | 132 / 132 | 8 / 8 | 一致 |
| `Timer2Isr` | 1,104 / 1,104 | 8 / 8 | 一致 |
| `DPRAMIsr` | 104 / 104 | 8 / 8 | 一致 |

结果为 `build/isr_binary_audit/result.json`，相关汇编、反汇编和编译日志放在同目录。普通 RAM 与函数的链接位置会因文件拆分改变，已链接机器码的地址重定位可使 ELF 字节哈希不同；本检查不宣称完整固件逐字节相同。符号指令一致、栈和尺寸保持提供目标编译器证据，仍不能推出板上中断最坏延迟或 cache/EMIFA 等待时间。

## 协议、参数和注释检查

协议回归将真实 `SstParams/SstParameterTable/SstParameterStore/PcDebugDispData/SstSerial/SstDebug/Comm2PcDebug/vofa/SstCommand` 分别编译再链接，硬件替身声明由 `tests/host_protocol_io.h` 提供。这样可以发现原来把多个 `.c` 拼在一起时被遮蔽的接口及链接错误。线上帧、映射、超时、CRC、流控制、RX64/TX16、RS485方向与存储互锁仍使用原断言。

参数精度检查只在唯一参数表单元中访问其私有条目，其他真实模块独立编译。其核验 37 个参数和 5 个持久化命令的线上编码、半 LSB 往返误差及第二次加载稳定性，不复制另一套参数表。

维护回归仍使用原假硬件布局和真实当前实现。假布局从 Git 原始头读取并记录 SHA256；这只用于测试存储，未恢复到目标工程。

`tests/comment_coverage.py` 独立检查函数五项中文说明及命名参数、变量、成员、宏、枚举注释。旧路径逐文件 token 一致率仅作为历史信息；重构的代码等价验收由上述独立搬运检查负责。

`tools/run_checks.py` 集中执行回归并记录源码/工程元数据指纹，`tools/delivery_manifest.py` 强制核验注释、搬运、结构及编译产物的报告与当前文件散列相符。实际 CCS 验证包含工程导入和 Debug/Release 的 clean、build，各步骤返回码记录于 `build/ccs_validation/result.json`。11 个宏配置的定义、编译及链接结果记录于 `build/matrix_result.json`。

最终证据分为三层：随 Git 版本提交的 [software_validation.json](evidence/software_validation.json) 提供便于审阅的摘要；`build/delivery_manifest.json` 提供当前工作区的输入和产物核对；各项 `build` 报告及日志保留具体断言和诊断。生成目录不进入源码提交，在新 clone 中按构建与验证文档重建，固定基线标签随仓库保留供等价审计使用。

软件源码与回归通过不能证明 FPGA 每个 100 μs 外部边沿均已满足截止期；缓存、EMIFA 等待及其他 ISR 延迟需按 `BOARD_ACCEPTANCE.md` 在实际板上对照记录。
