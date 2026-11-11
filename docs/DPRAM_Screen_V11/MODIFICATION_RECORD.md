# DPRAM屏幕兼容版修改记录

日期：2026-11-11。新分支 `compat/dpram-modbus-screen`；交付tag `dpram-screen-compat-v1.1-20261111`。

## 1. 归档与隔离

- 修改前现场源码、用户未提交调试配置和现有文档已整体提交为 `766fad0884695db011b0df77fce440ab3f5b40de`。
- 原工程tag：`before-dpram-screen-compat-20261111`；原分支已推送GitHub。
- 从该提交独立克隆到 `D:\Code\SST\AnyEyesVersion_DpramCompat`，新工程名及launch、产物同步改名；原V1源码未继续修改。
- 未复制.metadata或旧目标缓存；原工程当前Git工作区保持干净。

## 2. 源码变化

|模块|变化|
|---|---|
|SstDpramProtocol.h|集中57项参数、固定寄存器区域、权限/结果、后台接口和注释|
|SstDpramCommand.c|旧块1接实际请求，STOP/OFF优先、故障/存储互锁、手动开关与能力拒绝|
|SstDpramParameter.c|旧块2和普通参数区，signed PI、整组校验、实际回读、真实保护掩码写入|
|SstDpramTelemetry.c|Word状态/故障显式编码，修复OV/OC误标COMM/SYNC、S_Mode位移、无效反馈/二次采样、A相串联DC_V|
|SstDpramScreen.c|新增NOINIT尾区，REFRESH/APPLY/SAVE/LOAD、完成ACK、版本戳、每轮最多16字发布|
|SstDpramHost.c|撤出旧RemoteRegulate_Dispose_2活动路径；原块2只在初始化/受理后回写；分时通知、过滤无生产者插件|
|SstParameterTable.c|增加变化项提交入口；负PI二补码沿用唯一参数表，未修改浮点不被量化|
|ChbDabControl.c|只追加后台MaintenanceSwitch；停机/故障/真实反馈联锁，重复命令不重触发；既有控制实体未改|
|SystemInit/TaskSlice|初始化及后台调用；保留用户1ms通信节拍|
|DPRAM.C/C6748.cmd|块3使用新打包函数；新增0x66003300、0x320字节NOINIT区；旧地址不动|
|工程/工具|CCS独立名字与输出名；新增点表导出、通信/布局/维护开关测试、验证入口|

负PI：旧块2的组1 PI倍率依次为1e5/100/1000/1000；current_kp/ki共用dq且不额外反号。A5 5A串口维护协议不使用，485仍AnyEyes/VOFA；原纯DPRAM邮箱21接口与串口协议无关。

## 3. 保留的现场配置

单相、低压宏、现场参数、460800及1ms通信、原ISR GPIO测时和用户改动均保留。只把DPRAM默认改1、仿真器默认改0；开关输出/反馈仍0。

当前命令133V/模块、预充25V、运行限幅120A、快速OC120A/5拍、快速OV300V，原OC120/140A与OV300V均来自快照，不是本次重新推荐整定。

## 4. 验证结果

- 交付固件输入SHA256：`013a343b8ffa7c7443743aae67ff72e8237058327422a11b0b5df738e14e4d3f`。
- 修改前Debug：91个源文件、编译/链接成功、17个既有警告；日志保留在原工程build/Debug/before-dpram。
- 本次21项验证全部成功；包含6个构建组合与15组检查；验证期间固件输入保持一致。
- 6构建：Debug/Release默认；Debug AnyEyes、仿真器+DPRAM关闭、三相、开关输出/反馈启用。
- 新通信测试：单/三相、远控/仿真器、DPRAM开/关；穷举65536个电流Kp有符号编码；未改精度、整组拒绝、停止/故障优先、SAVE完成及失败、冲突和实际值回读。
- 新维护开关测试：单/三相、输出/反馈各合法组合；反馈1但输出0应编译期拒绝。验证停机/故障联锁、实际反馈要求、OFF优先、不重触发进线线圈、无PWM写入。
- 原控制68个既有实体有效代码哈希一致；ISR/ADC/ProtectionConfig逐字节一致；2400行地址换算/唯一性、57项参数及NOINIT布局通过。
- 全工程141个自定义源文件注释审查通过；串口协议范围、AnyEyes/VOFA、参数存储迁移、原DPRAM命令/维护、故障报告、PI、控制等所列回归通过。
- 真实SystemInit/TaskSlice/ISR分离编译的硬件边界测试：4配置各17个场景通过；更新了旧夹具以识别新增后台服务及现场已有的1ms节拍、停用录波调用。
- CCS实际导入、Debug及Release全量构建成功，固件SHA与独立构建一致；不是仅运行语法检查。
- 在同一独立CCS workspace中实际导入原AnyEyesVersionV1和新AnyEyesVersion_DpramCompat成功；未改动用户现用workspace。结果见CCS_PEER_IMPORT_RESULT.json。

|项|结果|
|---|---|
|Debug_default|PASS|
|Release_default|PASS|
|Debug_anyeyes|PASS|
|Debug_simulator|PASS|
|Debug_three_phase|PASS|
|Debug_switch_io|PASS|
|dpram_screen|PASS|
|dpram_screen_layout|PASS|
|comment_coverage|PASS|
|firmware_trace|PASS|
|dpram_commands|PASS|
|dpram_maintenance|PASS|
|parameter_precision|PASS|
|parameter_store_migration|PASS|
|protocol_regression|PASS|
|serial_protocol_scope|PASS|
|pi_regression|PASS|
|fault_reporting|PASS|
|control_regression|PASS|
|vofa_telemetry|PASS|
|dpram_switch_maintenance|PASS|

## 5. 继承失败项与验证边界

原V1在同一工具环境跑历史全套21组检查，有以下失败。本次没有为通过旧断言而恢复/重调现场控制算法；测试失败不能一概归因于测试过时，真实控制后果需另行逐项复核。

|历史检查|快照结果|
|---|---|
|realtime_audit|FAIL|
|refactor_equivalence|FAIL|
|framework_structure|FAIL|
|firmware_trace|FAIL|
|bench_inputs|FAIL|
|single_phase_control|FAIL|
|single_phase_io|FAIL|
|single_phase_isolation|FAIL|
|common_control_safety|FAIL|
|switch_io|FAIL|

上述中firmware_trace旧夹具缺1ms服务模拟，本版更新夹具后通过；comment_coverage基线通过且本版新增代码后也通过。其余历史来源一致性/控制/采样断言的继承失败不在本次通信交付中宣称解决。旧报告保存在BASELINE_HISTORICAL_REGRESSION_RESULT.json。

真实ARM/FSMC与Modbus屏幕未在线联调；没有主回路通电和GPIO最坏ISR测量。10kHz原函数保持、通信不上ISR，但EMIFA争用、后台响应及栈余量仍需控制板实测。新参数整组接口完全停机后提交，不在长临界区写整块参数。

旧屏幕PI控件倍率需按本版冻结值核对；预充/详细故障/SAVE补点需要绑定新寄存器。PQ/下垂/谐波/额外独立PI/其他电气端口未实现，不应被标记全功能支持。

## 6. 产物与校验

|产物|SHA256|
|---|---|
|build/Debug/default/SST_DpramCompat.out|`2d69e2b1fa7655f44c0d1d6b6b1890921be5fd4f5c869fb388c28bb0c549e5d5`|
|build/Debug/default/SST_DpramCompat.map|`b35b7c864b95f2f3ad13dfbc33065c889d6fecb561ce5b716295ae4da398ebb8`|
|build/Release/default/SST_DpramCompat.out|`23cb0b30236f5b5ae3cbf715caf2008b8efece61a200f431c72cc4822ea97bad`|
|build/Release/default/SST_DpramCompat.map|`a178a1349e7a8cf4cf48f9d15890def4912d9854cffbd43ff0eaaf254d019be3`|

详细日志、所有变体产物哈希见VALIDATION_RESULT.json、CCS_BUILD_RESULT.json及build/dpram_screen_validation；CCS原生输出在Debug/Release目录。

## 7. 联调入口

- 地址/倍率/权限：DPRAM_MODBUS_POINTS.csv、PARAMETERS.md。
- 现场操作、断点与验证：DEBUG_STEPS.md。
- 旧故障位与详细码：FAULT_CODES.md。
- 新固件TAG标识本次源码和文档；本文件不嵌入自身commit SHA，避免自引用。
