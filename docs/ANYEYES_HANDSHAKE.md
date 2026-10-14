# AnyEyes 电子标签、握手与版本查询

适用工程：`D:/Code/SST/AnyEyesVersion`。通信框架参考`D:/Code/CCSv12/F28377S_Plat`，硬件保持C6748 + FPGA。产品号、部件号和硬件平台是上位机选择产品配置的匹配信息；握手成功后才允许参数访问、波形订阅和状态订阅。握手成功本身不授予启停权限，启停仍由控制源宏决定。

## 参考工程的逻辑

参考`Source/04_App/00_Version/SoftVersion.cpp:23`集中定义四个电子标签：`PRODCT_SERI=255`、`PRODCT_PART=43`、`HARD_VERSION=1`、`OTHER_INFO=0`。同文件37～42行构成六段版本`项目.部件.平台.分支.发布.测试`，参考值为`255.43.1.0.0.6`；65行的构造函数把这些值装入版本对象，125行的`C_PRODUCT_INFO::ConstructEvent()`把四标签装入握手匹配对象。

参考`Comm2PcDebug.cpp:606`收到`FF`命令后比较四个字节，完全一致则置连接标志；四字节全为`FF`也允许通用连接。685行生成`7F`响应，依次发送匹配结果和六段软件版本。未连接时，491行的解析器仅接受握手和保留的重启入口。连接后，参数、波形、状态通过既有命令继续工作。

参考类中`CommVersion`实际来自`HARD_VERSION`，表示硬件平台；`McuType`实际来自`OTHER_INFO`，表示其他产品区分标记。这两个成员名不能解释为“协议版本”及“实际MCU型号”。本工程用明确的配置宏消除这一歧义。参考打印文本50～54行标明其产品是三电平DCDC、PCB为DG150101_C7S_V1.x；因此这些数字不能自行认定为所有SST产品通用的官方编号。本工程按用户确认保留`255/43/1/0`，供现有上位机产品配置继续匹配。

参考`FA`只设置`QuerySoftInfoFalg`，没有实现`7A`打包。本工程补齐`FA/7A`作为SST扩展；原`FF/7F`帧布局及既有参数、波形、状态地址保持不变。

## SST集中配置

配置文件为[SoftVersion.h](../Source/04_App/00_Version/SoftVersion.h)，实现文件为[SoftVersion.c](../Source/04_App/00_Version/SoftVersion.c)。修改或通过编译器预定义覆盖配置后，需重新编译并加载固件；标签不通过普通运行参数写入，也不随EEPROM参数恢复改变。

| 配置宏 | 当前值（十进制） | 线上字节 | 含义 |
|---|---:|---|---|
| `SST_PRODUCT_SERIES` | 255 | `FF` | 产品号/项目号 |
| `SST_PRODUCT_PART` | 43 | `2B` | 产品部件号 |
| `SST_HARDWARE_PLATFORM` | 1 | `01` | 硬件平台标识，沿用当前上位机配置 |
| `SST_VERSION_BRANCH` | 0 | `00` | 软件分支及握手第四区分标记 |
| `SST_VERSION_RELEASE` | 1 | `01` | 发布版本 |
| `SST_VERSION_BETA` | 0 | `00` | 内部测试版本 |

每项是0～255的单字节整数。当前四标签为`255,43,1,0`，六段版本为`255.43.1.0.1.0`。第四标签与版本分支使用同一配置值，延续参考`OTHER_INFO → BRANCH_VERSION`关系。发布或测试版本改变不会影响四标签匹配；项目、部件、平台或分支改变则需要同步上位机匹配配置。

`g_sst_soft_version`是唯一只读版本对象；CCS的Expressions可观察`product_series`、`product_part`、`hardware_platform`、`branch_version`、`release_version`、`beta_version`六成员。`SstVersion_MatchElectronicTag()`完成精确/通用匹配，`SstVersion_CopyBytes()`供握手与版本应答共用，避免匹配值和上报值各自硬编码而不一致。

当前用户源码配置为`SST_DEBUG_PROTOCOL=1`、`SST_BENCH_INPUT_SIM=1`、`SST_SIMULATOR_MODE=1`、`SST_ENABLE_DPRAM=0`。这套固件可握手、查询、停机调参及观测，但普通串口启动/复位命令不接管仿真器控制。测试输入接口和闭环测试步骤见[BENCH_CLOSED_LOOP_DEBUG.md](BENCH_CLOSED_LOOP_DEBUG.md)。切换VOFA后不解析AnyEyes握手；DPRAM启用与否不改变AnyEyes电子标签。

## 字节帧与CRC

```text
6B  LEN  CMD  PAYLOAD...  CRC8
```

`LEN=1+载荷字节数`，包含命令、不包含帧头、长度字段或CRC；完整帧字节数为`LEN+3`。CRC覆盖帧头至最后一个载荷字节，算法为多项式`0x07`、初值`0x00`、非反射、结果异或`0xFF`。电子标签和六段版本均逐字节发送，没有结构体填充及16位字节序问题；参数和波形中的16位数值仍按高字节在前发送。

| 方向/用途 | 完整十六进制帧 | 字段说明 |
|---|---|---|
| PC→UNIT，精确握手 | `6B 05 FF FF 2B 01 00 48` | 四标签=`255/43/1/0` |
| PC→UNIT，通用握手 | `6B 05 FF FF FF FF FF FD` | 全FF允许通用连接，不验证具体产品匹配 |
| UNIT→PC，握手成功 | `6B 08 7F 55 FF 2B 01 00 01 00 85` | `55`成功，后6字节为软件版本 |
| PC→UNIT，不匹配示例 | `6B 05 FF FE 2B 01 00 5E` | 把产品号改为254，其余不变 |
| UNIT→PC，握手失败 | `6B 08 7F AA FF 2B 01 00 01 00 C1` | `AA`拒绝，仍报告UNIT自身版本 |
| PC→UNIT，版本查询 | `6B 01 FA 2B` | 无载荷，仅连接后接受 |
| UNIT→PC，版本响应 | `6B 07 7A FF 2B 01 00 01 00 C8` | 6字节版本，无额外结果字节；SST扩展 |
| PC→UNIT，推荐心跳 | `6B 01 FE 37` | 无载荷；刷新已连接状态的活动时刻 |
| UNIT→PC，停机且无故障心跳示例 | `6B 04 7E FF AA 55 E2` | 第1字节兼容心跳标记FF；停机AA；无故障55 |

`7F`的载荷位置为`result,project,part,platform,branch,release,beta`，共7字节；`7A`仅包含后六个版本字节。失败响应的版本始终来自本机配置，不回显PC提供的错误标签。标签错误、载荷长度错误和CRC错误不授予连接权限。

## 连接、心跳与控制权限

PC先发`FF`，收到有效`7F`且结果`55`后再发`F0/F1/F2/F4/FA`。`AA`表示没有匹配当前UNIT配置，应检查四标签；通用连接仅用于明确选择通用模式的上位机配置，不能证明具体产品点表正确。

已连接时，合法命令以及`FE`心跳刷新接收活动时刻。连续超过30000ms没有合法命令则断链，停止通信订阅和待处理请求，再次访问需重新握手。PC建议每1s发送一次`FE`；PC无需对UNIT的`7E`另发应答。UNIT约每256ms上送一次`7E`，实际发送时间受后台负载、发送队列和485半双工传输影响。断链只解除调试通信，不自动停机，不取代原有软件保护及硬件FAULT处理。

每次载荷长度正确的`FF`新握手，以及30s连接超时，均清理前一会话的待处理读写、版本查询、波形和面板订阅。正确标签重新连接也需重新订阅；错误标签使当前连接失效并返回`AA`。握手应答成功入队前不处理其他业务应答，避免队列拥塞时先回复业务再报告握手结果。已经进入UART发送队列的完整旧帧继续按序发送，不在半帧中途清队列；测试重连时应根据收到的新`7F`及后续请求确认会话。运行命令不因这些会话清理动作而改变。

推荐PC使用上述无载荷`FE`帧；为了延续此前行为，本轮未对`FE`增加严格空载荷限制。CRC、整体帧长度和已连接条件仍需通过。

参考源码使用任务调用数计时，且部分注释与1ms实际调度不一致；本工程采用真实毫秒差计算30s超时。当前`7E`第一个载荷字节保持既有SST兼容值`FF`；参考原实现的`LifeTick++`在触发后的低8位实际为`00`，它不是电子标签或握手字段。

`7E`后两字节：任一AC或DAB PWM已使能为`55`、均禁用为`AA`；无当前故障为`55`、存在故障为`AA`。该心跳仅提供摘要，详细控制状态、故障码及参数仍读既有显示索引或功能码表。

握手仅证明标签匹配。`SST_ALLOW_SERIAL_COMMANDS=(!SST_SIMULATOR_MODE && !SST_ENABLE_DPRAM)`才允许普通串口启停/复位写；当前仿真器模式下，此类写拒绝码仍为5。普通停机调参受停机与存储忙互锁；急停保留现有受控入口。产品匹配逻辑不改PWM、TAD中断或控制策略。

## CCS与串口台架验证

1. 在CCS加载本轮实际构建的AnyEyes固件，确认`SST_DEBUG_PROTOCOL=1`和集中标签配置。让DSP连续运行；暂停DSP时后台串口服务停止，不能据暂停期间的无应答判定握手故障。当前临时输入测试配置的启停仍由仿真器控制。
2. 上位机或十六进制串口工具设置UART2的`115200、8数据位、无校验、1停止位`，通过UNIT实际485接口连接。参考F28377S串口为460800，不能照其波特率直接连当前SST。关闭自动追加CR/LF、文本转义及ASCII发送，按表发送真实十六进制字节。
3. 先发送精确握手帧，检查收到11字节成功响应、CRC正确、版本为`255.43.1.0.1.0`。按上位机的产品/设备配置方式填写十进制`255/43/1/0`或对应十六进制`FF/2B/01/00`，不可把“43”误当成十六进制`0x43`。
4. 发送`6B 01 FA 2B`，确认收到10字节`7A`响应，六版本字节与`7F`一致。发送`6B 03 F0 00 02 B4`读取原`0/2 ac_mode`，应返回`70`、同一组/索引及实际值；当ac_mode=1时完整应答为`6B 06 70 00 02 00 01 00 C6`。测试不需要发送启动命令。
5. 每1s发送`6B 01 FE 37`，观察周期`7E`；详细状态值与当前PWM/故障状态核对。按[ANYEYES_MAPPING.md](ANYEYES_MAPPING.md)开始/停止波形和状态订阅，确认地址及显示比例保持原表。485上采用请求—应答节奏，等完整应答后再发下一请求，避免与UNIT发送重叠。
6. 停止所有PC自动心跳、波形/状态订阅续发及其他合法命令，让DSP连续运行超过30s；再次发送`F0`或`FA`应没有业务响应，重新`FF`握手后恢复。UNIT自身发送`7E`不算收到PC心跳，不能保持连接。
7. 发送不匹配示例，检查`7F AA`，随后业务请求不应执行；重新正确握手后恢复。已有UART完整帧可能在新`7F`之前发完，应识别帧指令及边界，不能把旧帧当作失败握手后的新请求回复。单独验证全FF通用请求可连接。把正确握手末尾CRC由`48`改为`49`，应丢弃坏帧；后续正确帧能正常恢复。通过这些步骤分别验证匹配、拒绝、通用、坏帧恢复及断链重连。

参考串口启动阶段打印`ProjectName/PartName/SoftInfo/HardInfo/OtherInfo/BuildTime`，这些文本不参与匹配。本工程的匹配只使用二进制`FF/7F`及集中字段，不能用启动字符串代替握手。

## 本轮交付与软件复验

交付标签为`anyeyes-identity-2026-10-14`，修改前用户配置提交为`fc3c917`。当前AnyEyes测试固件为[build/Debug/anyeyes_identity/SST_AnyEyes.out](../build/Debug/anyeyes_identity/SST_AnyEyes.out)，CCS Debug/Release产物也已实际构建。本轮6种TI配置各89源文件编译、链接通过，CCS隔离导入和两配置通过，14项主机回归、5种协议配置、12种版本宏越界拒绝、38项测试输入隔离、8份map及132文件注释覆盖均通过。完整结果与源码/产物哈希见[anyeyes_identity_validation.json](evidence/anyeyes_identity_validation.json)。

上述是软件验证，尚未进行实际UNIT/RS485/AnyEyes客户端验收，也不证明完整100µs时限；台架操作按上一节逐项记录。测试输入宏关闭时ISR归一化汇编的隔离参照是`6675a72`，不是本轮通信修改前的`fc3c917`。

使用具备tree-sitter及C/C++语法包的Python、TI C6000 8.3.12、CCS 12.5和GCC，执行以下命令可重建本轮证据。各变体的`--define`仅对该次构建有效，当前源码仍保留用户的测试输入设置1。

```powershell
python tools/build.py --configuration Debug --variant anyeyes_identity
python tools/build.py --configuration Release --variant anyeyes_identity
python tools/build.py --configuration Debug --variant anyeyes_identity_normal --define SST_BENCH_INPUT_SIM=0
python tools/build.py --configuration Debug --variant anyeyes_identity_dpram --define SST_BENCH_INPUT_SIM=0 --define SST_SIMULATOR_MODE=0 --define SST_ENABLE_DPRAM=1
python tools/build.py --configuration Debug --variant anyeyes_identity_vofa --define SST_DEBUG_PROTOCOL=2
python tools/build.py --configuration Debug --variant anyeyes_identity_custom --define SST_PRODUCT_SERIES=201 --define SST_PRODUCT_PART=202 --define SST_HARDWARE_PLATFORM=203 --define SST_VERSION_BRANCH=204 --define SST_VERSION_RELEASE=205 --define SST_VERSION_BETA=206
python tools/ccs_validate.py
python tools/run_checks.py
python tests/bench_isolation.py
New-Item -ItemType Directory -Path build/anyeyes_identity -Force | Out-Null
python tests/dpram_map_audit.py build/Debug/anyeyes_identity/SST_AnyEyes.map build/Release/anyeyes_identity/SST_AnyEyes.map build/Debug/anyeyes_identity_normal/SST_AnyEyes.map build/Debug/anyeyes_identity_dpram/SST_AnyEyes.map build/Debug/anyeyes_identity_vofa/SST_AnyEyes.map build/Debug/anyeyes_identity_custom/SST_AnyEyes.map Debug/SST_AnyEyes.map Release/SST_AnyEyes.map --report build/anyeyes_identity/dpram_maps.json
python tools/export_anyeyes_validation.py
```

导出工具不重跑测试，只有报告指纹、协议/隔离输入哈希、map及固件均一致时才写入本轮摘要。构建目录和固件不进入Git，可按以上命令重建；历史验证摘要保留，不能代替当前源码的检查结果。
