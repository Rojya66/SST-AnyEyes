# SST_AnyEyes 工程

本工程在 C6748 + FPGA 硬件上迁移 `Chb_DabPrim_vofa` 的 HVAC/CHB 和 DAB 原边功能，按 `F28377S_Plat` 的六层结构组织。AnyEyes 使用参考工程 PcDebug 的命令、功能码地址格式、波形通道与状态索引格式；VOFA 是可选构建。

框架整理将入口、启动协调、后台任务、中断、数据定义及外围业务分别放回所属模块。先从 `Main.c`、`SystemInit.c`、`TaskSlice.c` 阅读执行顺序，再按数据域进入控制、采样、保护及通信模块；`main.H` 仅为历史兼容聚合头，活动模块直接包含所属接口。具体目录、调用关系及验证状态见 [框架整理交付说明](docs/FRAMEWORK_REFACTOR_RESULT.md)。

## 使用入口

1. CCS 12.5 导入本目录，工程名 `AnyEyesVersion`（固件文件名仍为 `SST_AnyEyes.out`），目标 TMS320C6748，编译器 C6000 8.3.12。
2. 编辑 `Source/04_App/00_Config/ChbBuildConfig.h`，或使用编译器 `--define` 覆盖宏。
3. 构建 Debug 或 Release；上电默认停机，初始化保持原 20 秒等待。
4. 仿真器查看或修改 `g_chb_dab_command`、`g_chb_dab_params`、`g_chb_dab_status`。开环/闭环由 `ac_mode` 选择。

默认配置为 **AnyEyes + 仿真器控制 + DPRAM 关闭 + EEPROM 自动加载关闭**。

| 宏 | 值 | 行为 |
|---|---|---|
| `SST_DEBUG_PROTOCOL` | 1 / 2 | AnyEyes / VOFA，必须二选一 |
| `SST_SIMULATOR_MODE` | 1 / 0 | 仿真器直接控制 / 允许远程控制 |
| `SST_ENABLE_DPRAM` | 0 / 1 | 不注册、不访问 DPRAM / 保留 DPRAM 遥控遥测 |
| `SST_ENABLE_MAINTENANCE` | 0 / 1 | 默认关闭 / 兼容 A5 5A 维护帧 |
| `CHB_LOAD_EEPROM_PARAMETERS` | 0 / 1 | 默认值启动 / 装载持久参数 |
| `SST_UART_BAUD` | 默认 115200 | UART2 波特率 |
| `SST_TELEMETRY_PERIOD_10MS` | 默认 2 | VOFA周期遥测间隔，单位10 ms；AnyEyes周期由请求设置 |

仿真器模式允许通信观测及停机参数访问，远程启动及普通运行控制被拒绝。非仿真器且 DPRAM 开启时，DPRAM 是运行命令来源，串口仍提供参数与观测；紧急停机仅请求停机。其余非仿真器配置由串口协议提供运行命令。

## 命令行构建

```powershell
python tools/build.py --configuration Debug
python tools/build.py --configuration Release
python tools/build.py --configuration Debug --variant vofa --define SST_DEBUG_PROTOCOL=2
python tools/build.py --configuration Debug --variant dpram --define SST_ENABLE_DPRAM=1 --define SST_SIMULATOR_MODE=0
```

脚本自动枚举工程源码并调用 TI 原生 C6000 编译器，可通过 `--tool-root` 或环境变量 `SST_CGT_ROOT` 指定位置。产物在 `build/<配置>/<变体>/`，包含 `.out`、`.map`、链接 XML、完整日志和 `build_result.json`。

## 文档

- [迁移与架构说明](docs/MIGRATION_GUIDE.md)
- [按 F28377S_Plat 框架整理的实施前方案](docs/FRAMEWORK_REFACTOR_PLAN.md)
- [框架整理交付说明与调用关系](docs/FRAMEWORK_REFACTOR_RESULT.md)
- [编译、宏配置及仿真器操作](docs/BUILD_AND_CONFIGURATION.md)
- [AnyEyes 参数、波形、状态对应表](docs/ANYEYES_MAPPING.md)
- [通信协议与串口实现](docs/COMMUNICATION.md)
- [DPRAM并口逐字地址、权限、数值位定义及功能覆盖](docs/DPRAM_MAPPING.md)
- [DPRAM完整地址CSV](docs/DPRAM_ADDRESSES.csv) · [DPRAM位定义CSV](docs/DPRAM_BITS.csv)
- [SST 故障码及参考故障机制](docs/FAULT_CODES.md)
- [构建与验证报告](docs/VALIDATION.md)
- [板级调试与 10 kHz 验收步骤](docs/BOARD_ACCEPTANCE.md)
- [自定义源码注释规范及覆盖核对](docs/SOURCE_COMMENTS.md)

本工程没有增加 DAB 副边、移相功率闭环或已删除的 HVDC/LVDC 功能。

## GitHub 版本区分

私有仓库：[Rojya66/SST-AnyEyes](https://github.com/Rojya66/SST-AnyEyes)。重构前版本为 `main` 及标签 `baseline-pre-framework-2026-10-12`，提交 `ddaeb56f21a81a70cdb76085728f91b67dc3c7a0`；重构后版本为 `refactor/f28377s-framework` 及标签 `framework-refactored-2026-10-12`。完整验证结果见 [VALIDATION.md](docs/VALIDATION.md)，可携带的软件证据见 [software_validation.json](docs/evidence/software_validation.json)。

CCS 工作区内克隆重构版本时使用目录名 `AnyEyesVersion`，与工程名保持一致：

```powershell
git clone --branch framework-refactored-2026-10-12 https://github.com/Rojya66/SST-AnyEyes.git AnyEyesVersion
```

标签克隆处于 detached HEAD，便于复现固定版本；继续开发可克隆 `--branch refactor/f28377s-framework`。`build/`、`Debug/`、`Release/` 是本地生成产物，不纳入源码提交。
