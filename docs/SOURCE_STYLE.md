# DSP 自定义源码排版与注释规范

本次整理日期：2026-10-15。整理前提交为 `1330a04`，工程为
`D:/Code/SST/AnyEyesVersion`。参考 `D:/Code/CCSv12/F28377S_Plat/Source` 的
Allman 大括号、语句展开、中文功能分区及函数说明块，建立可重复执行的排版规则。
参考工程本身混用了 Tab 和空格，本工程统一使用四个空格缩进。

## 整理范围

检查 `Source/` 下全部 132 个自定义 `.c/.C/.cpp/.h/.H/.hpp` 文件，覆盖公共算法、
FPGA、DPRAM、存储、串口、板级初始化、主循环、中断、配置、版本、参数、采样、
PcDebug、控制、保护及并口主机服务。
TI 设备头文件 `Source/00_Device/` 和驱动库 `Source/01_Drive/`、汇编、历史归档及
构建生成文件不参与批量格式化。

## 排版规则

- 函数、类型和已有控制块的大括号换行；每层缩进四个空格。
- 每条语句、每个枚举成员独立显示，不把判断、赋值、`break` 或 `return` 压在同一行。
- 短函数、短控制块和循环不折叠。保留原有单语句控制体的语法，不自动插入或删除大括号。
- 二元运算符和逗号后保留空格，长表达式和调用参数按 120 列目标换行。
- 原有逗号声明保留同一类型和声明顺序，每个变量及其现有注释分别显示；
  不为排版改动类型、限定符、初值或对象布局。
- 文件包含、宏定义、私有类型、变量、功能实现用中文分区。
  不自动重排 `#include`、宏、声明或函数。
- UTF-8 中文、LF 行尾；清理行尾空白和缩进中的 Tab。
  函数说明块和协议位定义保留人工布局，不对中文长段落盲目自动重排。

根目录 `.clang-format` 固定基础规则。`tools/format_source.py` 另外把既有具名变量
注释移到对应声明项旁，并在写入前比较词法内容和预处理指令边界。

## 注释规则及本次修正

函数说明清楚交代实际功能、输入和返回值；有指针输出、外设写入、共享状态更新或
调用上下文限制时补充说明。`void` 接口不强制堆叠“无输出、无返回”等空模板。
参数、变量及结构成员在相邻位置写清用途，能够确定的量注明单位、范围、计时粒度及
线上编码。允许具名注释，也允许变量同行的明确中文说明，不要求重复变量名。

主要人工复核内容如下：

- `Comm2PcDebug.cpp/.hpp`：按接收解析、会话、发送和周期上报分区，展开全部命令分支；
  说明 CRC8 的字节长度、初值、多项式和最终取反，明确保留命令、握手标签、心跳及队列行为。
- PcDebug 显示表、调试服务及 VOFA：补充通道分组和单位，纠正 JustFloat 帧被描述为
  “含帧头及校验”的错误，区分历史串口缓存和当前实际通信职责。
- DPRAM、参数存储和保护：修正字/字节单位、CRC 位循环、EEPROM 提交与读回校验次序、
  软件调用层的宏门控职责，以及内部故障环与调用者缓存的区别。
- 控制、串口和中断：说明当前临时输入注入宏为 1、主循环收发预算及硬件保护依赖现场位流；
  不再把调制中点描述为单 UNIT 下必然零电压。恢复了遗留的 `???` 破损注释。

已存在的作者和版权信息保留，不为迁移代码虚构作者或历史日期。

## 工具使用

日常 CCS/TI 编译不依赖格式化或语法扫描工具。需要整理源码时安装工具：

```powershell
python -m pip install clang-format==20.1.8 tree-sitter tree-sitter-c tree-sitter-cpp
python tools/format_source.py --check
python tools/format_source.py --write
python tools/format_source.py --check --verify-ref 1330a04
python tests/comment_coverage.py
```

如 `clang-format` 不在 PATH，用 `--clang-format` 指定其可执行文件。
本次临时工具安装在忽略的 `build/style_tools/`，脚本也会查找其中的格式化器；
使用该目录的语法包时将它加入 `PYTHONPATH`。依赖未写入项目源码或固件。

注释覆盖审计现在按语法树定位每个声明项，支持前置、尾随及逗号续行注释；
枚举也接受同行说明。仍检查函数、参数、变量、成员和宏的文档覆盖，具体语义由人工复核。

旧等价审计把 C 的反斜杠续行当成可执行 token，本次纠正为先执行物理行拼接再比较。
只有既有 `SVG_Udc_Average` 合同的两项哈希受此算法修正影响：已确认整理前 `1330a04`
快照和当前函数在新算法下得到相同 current 哈希，再更新对应哈希；原理由和回归要求保留。
没有通过新增行为例外来接受本次排版。

## 验证与证据

当前结果见 `docs/evidence/source_style_validation.json`。完整日志在忽略的 `build/` 中：

- `build/style_audit/source_preservation.json`：132 个文件相对 `1330a04` 的词法及指令边界对照。
- `build/style_audit/final_style.json`：格式检查及排版筛选指标。
- `build/style_audit/firmware_sections.json`：同一 TI 工具链下，整理前后默认 Debug 的已分配 ELF 节对照。
- `build/comment_inventory/verification.json`：注释覆盖审计。
- `build/host_validation/`：控制、通信、故障、DPRAM、参数、框架和临时注入等主机回归。

本次排版筛选中，同一行含多条语句的代码行由 124 行降为 0，超过 120 列的代码行由
121 行降为 0（排除注释和 `for` 头的分号）。默认 Debug/Release 及 VOFA+DPRAM
构建均通过，保留与整理前相同的 17 条既有编译告警；14 项主机回归全部通过。
默认 Debug 的全部 34 个已分配 ELF 节与整理前一致。

固件对照覆盖指令、常量、初始化数据及 RAM/MMIO 布局；源码行号和调试信息可以变化，
因此不能用整个 `.out` 文件的 SHA256 不同来判断控制逻辑变化。
该对照不代替实板的 10 kHz 中断时延、WCET 或漏沿测量。

CCS 使用原工程继续编译；本次 CLI 固件分别位于
`build/Debug/style_after/SST_AnyEyes.out` 和 `build/Release/style_after/SST_AnyEyes.out`。
协议及保护参数地址、默认配置和当前调制算法均沿用整理前版本。

在日志仍保留时，可运行 `python tools/export_style_validation.py` 重新核验并导出当前证据；
脚本会拒绝源码哈希不匹配、构建中源码变化、检查失败或过期的记录。
