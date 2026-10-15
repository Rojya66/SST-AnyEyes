# 自定义源码注释说明

2026-10-15 已按参考工程重新整理全工程排版和注释，当前规则、工具与验证说明见
[DSP 源码排版规范](SOURCE_STYLE.md)。下文旧计数及固定版本验证保留为历史记录。

2026-10-13并口维护新增模块也按此规范说明上下文、数值缩放、权限、CRC、版本戳和分片预算；485 MaintenanceProtocol已移除。当前覆盖/构建结果见 [dpram_maintenance_validation.json](evidence/dpram_maintenance_validation.json)，下文较早的计数和等价结果对应历史框架交付，不能沿用为当前源码验收。

按 `F28377S_Plat` 的横线函数说明块维护 `Source` 中的全部自定义代码注释。框架拆分后的范围包括入口、启动、后台调度、中断、板级适配、各域数据与初始化、原 SST 控制、采样、保护、FPGA、DPRAM、通信和存储模块。TI 设备文件 `Source/00_Device/`、TI 驱动库 `Source/01_Drive/` 不属于本次自定义注释范围。最终文件和声明计数见 [随版本验证摘要](evidence/software_validation.json) 及 `build/comment_inventory/verification.json`。

## 注释格式

函数定义和对外声明说明功能、输入和实际返回值；有输出、副作用或调用上下文限制时补充“输出参数、执行说明”。输入参数逐个列出；输出说明指针写入目标及主要全局/硬件副作用。void 接口不强制重复无输出、无返回的空模板。不添加无法核实的作者或创建日期。

变量、局部计算缓存及结构体成员在声明相邻位置以中文注释说明用途，允许具名或明确的同行说明。同一声明中的多个变量分别换行、逐个说明。可确认的电压、电流、频率、角度分别标明 V、A、Hz、rad；软件计时区分 ms、10 ms 节拍和 TAD 次数；FPGA 占空比、周期及死区使用原硬件计数。

参数映射另说明线上16位编码、缩放比例、符号和受控目标。原DPRAM成员区分物理量与旧协议编码，原区完整地址见 [DPRAM_MAPPING.md](DPRAM_MAPPING.md)，新54点和快照见 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)；AnyEyes独立组/索引见 [ANYEYES_MAPPING.md](ANYEYES_MAPPING.md)。保留字段、原拼写、未执行查询和空VOFA入口按实际实现说明，不宣称额外功能。

宏、枚举常量也补充语义，尤其是协议二选一、DPRAM 使能、仿真器命令权限、固定参数地址容量及五位故障码。原作者署名和历史注释保留。

## 覆盖结果与行为不变核对

最终语法扫描无缺失项。统计包括函数定义、函数声明、具名参数、变量声明、结构体/联合体成员、宏和枚举。数字表示源码出现次数，含 extern、头文件声明和未选择的条件编译端口分支，不是固件中独立链接符号数；框架拆分增加了窄接口和所属头文件，不能继续使用迁移前 46 文件的旧统计。

补注释前的历史文件级比较仅描述此前补注释阶段。当前框架已搬动函数、对象和头文件，旧路径 token 不一致不等于行为改变；本轮执行等价性由 `tests/refactor_equivalence.py` 相对 Git 标签 `baseline-pre-framework-2026-10-12` 按实体比较。函数、对象、类型、宏、条件和硬件段分别核对；经审阅的接口变化记录精确旧新哈希和对应回归，ISR 不允许使用例外合同。文件编码使用 UTF-8，中文编辑器需按 UTF-8 打开。

注释说明同时接受职责复核，不能仅依靠“有中文”判断正确。例如 `Port_StateUpdate_Write` 明确其 FPGA `HVAC_STATUS`、故障位、LED 和 GPIO 副作用；`Port_RealTimeParameter` 明确其主循环测量整理上下文；`Variable_RST` 明确启动及 PWM 开锁路径；模块数量说明与当前 A 相 1 个、B/C 相各 13 个的有效位图一致。中断注册归 `Redef_Interrupt`，组合毫秒时基实现归 `CpuTask`，UART 配置和有界收发归 `SstSerial`。

初始化通过展开所属域函数比较原执行语句及预处理指令：`Initial_Variable` 为 3,120 个执行 token，`Variable_RST` 为 175 个执行 token，次序严格保持。实时审计继续核验 ISR 顺序、10 kHz 调用链、GPIO 向量及 DPRAM 宏门控。该检查允许注释和空白变化，拒绝未审阅的运算符、标识符、常量或可执行语句变化。

覆盖及等价报告：`build/comment_inventory/verification.json`。日常编译不依赖注释工具。需要重跑完整语法覆盖检查时：

```powershell
python -m pip install tree-sitter tree-sitter-c tree-sitter-cpp
python tests/comment_coverage.py
```

后续直接维护所属源文件和公开头的接口契约，再运行覆盖与等价审计。`tools/comment_inventory.py` 和 `tools/comment_semantics.py` 提供语法清单及具名说明检查，日常构建不依赖历史注释缓存，也不通过生成器覆盖当前功能代码。

2026-10-12框架交付完成TI Debug/Release、实际CCS导入、11种宏组合、主机回归、独立头编译、3×17启动/调度/ISR轨迹及四个ISR指令/栈/尺寸对照。该阶段计数和产物哈希见 [software_validation.json](evidence/software_validation.json)，当前并口调整见本页开头的新证据。注释等价及编译不能代替板上100µs时序实测，步骤见 [板级验收](BOARD_ACCEPTANCE.md)。
