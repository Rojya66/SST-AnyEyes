# 随版本软件验证证据

当前并口维护调整（2026-10-13）的摘要是 [dpram_maintenance_validation.json](dpram_maintenance_validation.json)：删除485 MaintenanceProtocol、DPRAM54点/快照、v2参数存储，以及本轮构建和主机检查。其结果须匹配当前源码指纹；本文下面的 `software_validation.json` 和保护注释修正是固定历史版本，不能代替当前验证。

保护注释后续修正见 [protection_comment_correction.json](protection_comment_correction.json)：原保护计时单位、回差及测量口径说明已纠正，全部固件输入的执行token与预处理指令相对已编译版本保持一致。本次只运行注释覆盖及源码等价核验，没有重跑目标构建。下面的完整软件验证摘要固定对应 `framework-refactored-2026-10-12`，其中原始文件字节哈希不代表修正注释后的源码。

`software_validation.json` 是2026-10-12框架发布源码的历史摘要。它记录固定基线、源文件指纹、目标构建和宏配置、CCS导入、主机检查、初始化等价、注释覆盖、头文件编译、启动/后台/ISR轨迹、ISR编译对照、硬件map、内存用量和15份固件哈希。

`report_sha256` 指向本机 `build/` 中的原始报告；主机检查同时保留日志SHA256。完整日志及固件为可重新生成产物，不进入Git。不同路径、时间戳或调试信息会影响重建产物的字节哈希，核验具体结果时同时比较编译配置、源码指纹和对应报告。

先按 [VALIDATION.md](../VALIDATION.md) 运行构建、回归、ISR和map审计，再执行：

```powershell
python tools/delivery_manifest.py
python tools/export_validation_summary.py
```

生成清单前需运行 `python tests/dpram_point_table_audit.py`，独立核对点表、固定旧地址和TI类型大小。当前导出默认生成 `dpram_maintenance_validation.json`，对应标签 `dpram-maintenance-2026-10-13`，保留旧 `software_validation.json`；其他版本可用 `--output 文件名.json --revision-tag 标签名`。

导出只在交付清单已通过且源码指纹仍相符时执行。摘要的 `physical_board_validation_performed=false` 表示没有进行板上时序验收，不能用软件报告证明完整100 μs截止期。
