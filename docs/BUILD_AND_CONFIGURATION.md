# 编译、模式与使用说明

## 工程导入与产物

工程目录为 `D:/Code/SST/AnyEyesVersion`，CCS工程名为 `AnyEyesVersion`，固件文件名保持 `SST_AnyEyes.out`，目标为TMS320C6748。使用CCS 12.5与TI C6000 8.3.12，原 `C6748.cmd` 的FPGA/DPRAM链接映射保持不变。

框架整理后从 `Source/03_System/01_OS/Main.c` 阅读入口，从 `SystemInit.c` 阅读初始化顺序，从 `TaskSlice.c` 阅读后台顺序。原 `main_485test.c` 不再参与构建，历史 UART/按键路径归档在 `archive/legacy/`；活动 `Source/` 下的文件由脚本递归枚举，CCS 工程配置也需随实际模块目录同步。重构后的当次构建状态见 `docs/FRAMEWORK_REFACTOR_RESULT.md`，不能沿用旧日志认定新源码已经编译通过。

在CCS选择 Project → Import CCS Projects，搜索本工程目录并导入。选择Debug或Release后 Build Project，产物为 `Debug/SST_AnyEyes.out` 或 `Release/SST_AnyEyes.out`。下载前核对目标配置、模块位图、FPGA版本及所用宏。首次上电默认停机，保持原20秒等待。

### 导入报错：Invalid project description

2026-10-12在工作区 `D:/Code/SST` 导入时，`.metadata/.log` 的具体错误为：

```text
D:\Code\SST\AnyEyesVersion overlaps the location of another project: 'AnyEyesVersion'
```

原工程名称是 `SST_AnyEyes`，目录名称是 `AnyEyesVersion`。目录直接位于工作区根目录下时，Eclipse的工程位置校验会与默认工程位置发生冲突；即使Project Explorer没有显示 `AnyEyesVersion`，仍可能出现该错误。本次在相同结构的临时工作区复现了同一错误，并验证工程名与目录名一致后可直接导入。

为保留当前工作区 `D:/Code/SST`，已将 `.project` 中的工程名改为 `AnyEyesVersion`，并将 `.cproject` 中Debug/Release的产物名称、链接输出、map和链接XML名称固定为 `SST_AnyEyes`，因此固件路径保持兼容。使用步骤：

1. 关闭Operation Summary和原导入对话框，重新选择 **Project → Import CCS Projects**，让CCS重新读取已修改的工程描述。
2. 搜索 `D:\Code\SST\AnyEyesVersion`，应显示 `AnyEyesVersion`。
3. 选择该工程，取消 **Copy projects into workspace**，点击Finish。在原工作区选择Debug或Release并执行Build Project。

不需要切换工作区、移动源码目录或复制工程。本次修复没有修改控制源码、中断配置或硬件映射。原工程描述备份位于 `build/workspace_import_validation/original_metadata/`，复现与修复后的导入/编译日志位于 `build/workspace_import_validation/`；`tools/verify_workspace_import.py` 可再次执行该检查。

若其他工作区确实已注册一个指向同目录的旧工程，可在Project Explorer中移除旧工程的工作区记录：Delete对话框中必须取消 **Delete project contents on disk**，然后重新导入。不要删除源码目录或整个 `.metadata`。之前临时建立的独立工作区不是本次修复的使用入口。

命令行构建无需打开CCS：

```powershell
python tools/build.py --configuration Debug
python tools/build.py --configuration Release
python tools/build_matrix.py
python tools/ccs_validate.py
```

`build.py`可用 `--tool-root` 或环境变量 `SST_CGT_ROOT`指定编译器目录；默认路径为 `E:/Applications/ti/ccs1250/ccs/tools/compiler/ti-cgt-c6000_8.3.12`。脚本每次重新编译当前源码并链接，保留 `.out`、`.map`、链接XML、日志和JSON结果。CLI产物在 `build/<配置>/<变体>/`，与CCS的Debug/Release目录区分。

`ccs_validate.py`在系统临时目录建立独立工作区，再调用CCS实际导入和构建应用。它的日志位于 `build/ccs_validation/`；默认eclipsec路径为 `E:/Applications/ti/ccs1250/ccs/eclipse/eclipsec.exe`，移动工具链后需更新该脚本或使用CCS界面导入。工程元数据包含C++ PcDebug与C控制模块，并保留CCS所需的XML版本处理指令。

Debug使用原`-g`，Release沿用原工程关闭优化的策略。两者都没有为了通信迁移提高控制算法优化等级；Release名称不代表本次新增实时性能优化。映射DPRAM的section使用`NOINIT`，C运行库不在EMIFA初始化前清零外部通信区；DPRAM启用后由原后台收发使用实际内容，普通RAM缓存仍按原初始化。

## 宏与控制来源

修改 `Source/04_App/00_Config/ChbBuildConfig.h`，或在CCS编译器预定义符号中覆盖宏。命令行示例：

```powershell
python tools/build.py --variant anyeyes_remote --define SST_SIMULATOR_MODE=0
python tools/build.py --variant vofa_sim --define SST_DEBUG_PROTOCOL=2
python tools/build.py --variant anyeyes_dpram --define SST_ENABLE_DPRAM=1 --define SST_SIMULATOR_MODE=0
```

| 选择 | 默认 | 说明 |
|---|---:|---|
| `SST_DEBUG_PROTOCOL` | 1 | 1=AnyEyes，2=VOFA；不能同时发送两种连续遥测 |
| `SST_SIMULATOR_MODE` | 1 | 1=通过仿真器直接写控制命令；0=允许远程控制 |
| `SST_ENABLE_DPRAM` | 0 | 独立启用DPRAM中断、后台读写和原布局 |
| `SST_ENABLE_MAINTENANCE` | 0 | 可选原A5 5A维护帧，与所选遥测协议共用整帧队列 |
| `CHB_LOAD_EEPROM_PARAMETERS` | 0 | 1=初始化停机时载入新SST参数；非法图像拒绝并保留默认值 |
| `SST_UART_BAUD` | 115200 | UART2，8N1；AnyEyes限频也使用该值 |
| `SST_UART_INPUT_CLOCK_HZ` | 228000000 | 与原UART2实际输入时钟一致，板卡时钟变化需核对 |
| `SST_TELEMETRY_PERIOD_10MS` | 2 | VOFA周期，默认20ms即50Hz；AnyEyes由请求设置周期 |

| 仿真器宏 | DPRAM宏 | 运行命令来源 | 串口观测/停机调参 |
|---:|---:|---|---|
| 1 | 0 | 仿真器 | 可用 |
| 1 | 1 | 仿真器；DPRAM仅通信兼容，远程启动不生效 | 可用 |
| 0 | 0 | AnyEyes控制命令；VOFA构建可选维护控制 | 可用 |
| 0 | 1 | DPRAM；串口普通启动/复位拒绝 | 可用 |

AnyEyes FC急停始终清运行命令。VOFA本身是单向JustFloat观测格式，没有虚构VOFA启停报文；需要远程控制时可启用原维护协议或DPRAM。两种保存任务共享忙标志，启动/修改在保存期间拒绝，停止/关闭仍有效。仿真器直接写RAM无法被通信权限机制拦截，调试期间由操作者遵守停机调参要求。

旧 `CHB_CONTROL_MODE` 常量仅为共享源码兼容。正常选择应使用上述独立宏。`CHB_OPEN_LOOP_PULSE_TEST`指旧透明EMIF桥，本工程不启用；普通开环逆变通过 `ac_mode=0` 完整保留。

## 仿真器开环、闭环与DAB

监视 `g_chb_dab_command`、`g_chb_dab_params`、`g_chb_dab_status`。先确认两个使能均为0、输出已关闭、参数与硬件相符。保持原模块位图：A相1个，B/C各13个；硬件安装不同时先修改 `ChbDabControl.h` 并重新编译。

| 操作 | 命令/观测 |
|---|---|
| CHB开环 | `ac_mode=0`；默认调制比0.20、50Hz；设置`run_enable=1` |
| 独立DAB原边 | `dab_enable=1`，默认占空计数350逐TAD递增至2500 |
| 闭环 | 停机设`ac_mode=1`，核对预充、PLL、PI与保护参数后设置`run_enable=1`；按需要启用DAB |
| 闭环策略 | `startup_strategy=0`为原兼容策略；1为Id斜坡后接管电压环 |
| 停止 | 将`run_enable`、`dab_enable`清零，确认PWM状态关闭 |
| 故障复位 | 停机且运行命令清零时设`fault_reset=1`；原核心消费后自动清零，不自动重新运行 |

实际功率试验顺序、外部TAD与故障封波测量见 [BOARD_ACCEPTANCE.md](BOARD_ACCEPTANCE.md)。参数表中的 `sample_period_s`在线只读，固定算法积分步长不代表FPGA物理触发已达到100us。

## 交付文件

| 内容 | 位置 |
|---|---|
| CCS工程/硬件映射 | 根目录`.project/.cproject/.ccsproject/C6748.cmd`、`targetConfigs/` |
| 控制、通信、故障及原驱动 | `Source/` |
| AnyEyes接入说明与可导入表 | `docs/ANYEYES_MAPPING.md`、`docs/ANYEYES_PARAMETERS.csv` |
| 故障来源、编码、等级与历史 | `docs/FAULT_CODES.md` |
| 分层迁移及功能边界 | `docs/MIGRATION_GUIDE.md` |
| 框架整理前后对应及调用关系 | `docs/FRAMEWORK_REFACTOR_RESULT.md`、`docs/REFACTOR_OBJECT_MOVES.json` |
| 框架整理实施前方案 | `docs/FRAMEWORK_REFACTOR_PLAN.md`（保留为历史方案） |
| 旧非活动路径 | `archive/legacy/`，在 `Source/` 外，不进入构建 |
| 构建记录与软件验证 | `docs/VALIDATION.md`、`build/` |
| 固件校验清单 | `build/delivery_manifest.json`（源码输入与15个固件产物SHA256） |
| 板级验收 | `docs/BOARD_ACCEPTANCE.md` |
| 构建和回归脚本 | `tools/`、`tests/` |

当前工程提供源代码和可构建固件，不附带AnyEyes客户端工程配置文件；客户端根据对应表配置原地址即可。实板时序、EEPROM掉电行为、RS485电气及客户端连接需按验收文档记录。

## 从 GitHub 取得与区分版本

专用私有仓库为 [Rojya66/SST-AnyEyes](https://github.com/Rojya66/SST-AnyEyes)，需要账户具有该私有仓库访问权限。若克隆目录直接位于 CCS 工作区根目录，使用 `AnyEyesVersion` 作为目录名，以匹配 `.project` 工程名：

```powershell
git clone --branch framework-refactored-2026-10-12 https://github.com/Rojya66/SST-AnyEyes.git AnyEyesVersion
```

该命令取得固定发布标签并处于detached HEAD；继续开发时使用 `--branch refactor/f28377s-framework`。重构前版本为 `main` 及标签 `baseline-pre-framework-2026-10-12`，提交 `ddaeb56f21a81a70cdb76085728f91b67dc3c7a0`。重构后分支为 `refactor/f28377s-framework`，标签为 `framework-refactored-2026-10-12`。基线标签不移动、不覆盖；切换版本前保持工作区干净。

`build/`、`Debug/`、`Release/` 不纳入源码 Git 提交；它们由当前版本的构建和回归命令重新生成。框架源等价审计从固定 Git 标签读取基线，克隆后保留该标签；无需把旧固件的中间文件当作基线源码。
