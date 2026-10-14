# 随版本软件验证证据

当前电子标签与握手版本（2026-10-14）的证据是 [anyeyes_identity_validation.json](anyeyes_identity_validation.json)，由`tools/export_anyeyes_validation.py`核对6种TI构建、CCS导入和两个构建、14项主机回归、5种协议配置及12种非法宏、38项隔离、8份map和132文件注释哈希后生成。修改前提交为`fc3c917`，最终标签为`anyeyes-identity-2026-10-14`；隔离对照仍使用固定`6675a72`，两种参照用途不同。源码指纹、输入哈希及产物哈希需一致，导出遇到旧报告会拒绝写入。

此前单UNIT输入测试（2026-10-14）的证据是 [bench_input_validation.json](bench_input_validation.json)，由 `tools/export_bench_validation.py` 在该版本构建和检查完成后生成。其测试前固定提交为 `6675a72`。该文件及并口维护、框架发布摘要保留为历史版本，不能代替当前电子标签版本的结果。

2026-10-13并口维护固定版本的摘要是 [dpram_maintenance_validation.json](dpram_maintenance_validation.json)：删除485 MaintenanceProtocol、DPRAM54点/快照、v2参数存储，以及该版本构建和主机检查。其源码指纹对应当时的标签；它与下面的 `software_validation.json`、保护注释修正均是历史记录，不能代替当前输入测试验证。

保护注释后续修正见 [protection_comment_correction.json](protection_comment_correction.json)：原保护计时单位、回差及测量口径说明已纠正，全部固件输入的执行token与预处理指令相对已编译版本保持一致。本次只运行注释覆盖及源码等价核验，没有重跑目标构建。下面的完整软件验证摘要固定对应 `framework-refactored-2026-10-12`，其中原始文件字节哈希不代表修正注释后的源码。

`software_validation.json` 是2026-10-12框架发布源码的历史摘要。它记录固定基线、源文件指纹、目标构建和宏配置、CCS导入、主机检查、初始化等价、注释覆盖、头文件编译、启动/后台/ISR轨迹、ISR编译对照、硬件map、内存用量和15份固件哈希。

`report_sha256` 指向本机 `build/` 中的原始报告；主机检查同时保留日志SHA256。完整日志及固件为可重新生成产物，不进入Git。不同路径、时间戳或调试信息会影响重建产物的字节哈希，核验具体结果时同时比较编译配置、源码指纹和对应报告。

当前电子标签版本的复验及导出命令见[ANYEYES_HANDSHAKE.md](../ANYEYES_HANDSHAKE.md)。下列命令属于原完整交付工具链；先按对应版本的 [VALIDATION.md](../VALIDATION.md) 运行构建、回归、ISR和map审计，再执行：

```powershell
python tools/delivery_manifest.py
python tools/export_validation_summary.py
```

生成清单前需运行 `python tests/dpram_point_table_audit.py`，独立核对点表、固定旧地址和TI类型大小。当前导出默认生成 `dpram_maintenance_validation.json`，对应标签 `dpram-maintenance-2026-10-13`，保留旧 `software_validation.json`；其他版本可用 `--output 文件名.json --revision-tag 标签名`。

导出只在交付清单已通过且源码指纹仍相符时执行。摘要的 `physical_board_validation_performed=false` 表示没有进行板上时序验收，不能用软件报告证明完整100 μs截止期。
