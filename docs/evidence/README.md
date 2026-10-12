# 随版本软件验证证据

保护注释后续修正见 [protection_comment_correction.json](protection_comment_correction.json)：原保护计时单位、回差及测量口径说明已纠正，全部固件输入的执行token与预处理指令相对已编译版本保持一致。本次只运行注释覆盖及源码等价核验，没有重跑目标构建。下面的完整软件验证摘要固定对应 `framework-refactored-2026-10-12`，其中原始文件字节哈希不代表修正注释后的源码。

`software_validation.json` 是本次最终源码的软件验证摘要，随重构版本提交。它记录固定基线、源文件指纹、目标构建和宏配置、CCS导入、主机检查、初始化等价、注释覆盖、头文件编译、启动/后台/ISR轨迹、ISR编译对照、硬件map、内存用量和15份固件哈希。

`report_sha256` 指向本机 `build/` 中的原始报告；主机检查同时保留日志SHA256。完整日志及固件为可重新生成产物，不进入Git。不同路径、时间戳或调试信息会影响重建产物的字节哈希，核验具体结果时同时比较编译配置、源码指纹和对应报告。

先按 [VALIDATION.md](../VALIDATION.md) 运行构建、回归、ISR和map审计，再执行：

```powershell
python tools/delivery_manifest.py
python tools/export_validation_summary.py
```

导出只在交付清单已通过且源码指纹仍相符时执行。摘要的 `physical_board_validation_performed=false` 表示没有进行板上时序验收，不能用软件报告证明完整100 μs截止期。
