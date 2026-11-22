# DPRAM有效掩码解析修复记录

日期：2026-11-13。工程：`D:\Code\SST\AnyEyesVersion_DpramCompat`。

本文件记录当日先完成的掩码修复构建。随后按用户要求解除HVAC START对DAB的启动联动，更新了源码注释和遥调回读许可；当前行为、映像及测试以 [DAB_COMMAND_AND_ASSIGNMENT_20261113.md](DAB_COMMAND_AND_ASSIGNMENT_20261113.md) 为准。下文编译指纹属于先前掩码修复映像。

## 1. 现场现象及原因

屏幕提交邮箱1后，DSP已进入控制块处理，但`SstDpramCommand_ProcessControlBlock()`返回`SST_SCREEN_UNSUPPORTED=7`，`g_sst_dpram_screen_result.error_word=0`。现场`COM_START_PORT16`原值为49409，即`0xC101`；此前还记录过2385，即`0x0951`。

原解析器直接用原始16字逐字检查“本机支持位”。word0是保留字，只要非零，就在检查启动字前拒绝整包。此外，启动字的高位保留内容及协议合法的传感器/电网描述也会导致同类拒绝。此处混淆了“协议保留位”和“协议有效但本机没有实现的功能”。

按Word的启动有效掩码`0x000F`：

```c
0xC101 & 0x000F = 0x0001; /* HVAC启动。 */
0x0951 & 0x000F = 0x0001; /* HVAC启动。 */
```

以上两种输入都不应因为保留位而丢失有效HVAC启动请求。本次在接收端兼容忽略保留内容；发送端仍建议按Word将保留内容写0，并避免重复携带上笔的有效动作。

## 2. 控制块修改

生产文件：`Source/04_App/10_Host/SstDpramCommand.c`。

新增`s_control_valid_masks[16]`。入口先将原始16字按Word掩码提取到`command_words[16]`，后续STOP/OFF判断、本机能力检查、互斥、故障/忙联锁、开关许可、统一控制入口全部使用提取后的值。

|word / Modbus|字段|有效掩码|
|---|---|---|
|0 / 00F0、1 / 00F1|Reserved_1|0000|
|2 / 00F2|COM_START_PORT|000F|
|3 / 00F3|COM_STOP_PORT|000F|
|4 / 00F4|COM_PORT_MODE|3FFF|
|5 / 00F5|Reserved_2|0000|
|6 / 00F6、7 / 00F7|COM_SWITCH_ON/OFF|000F|
|8 / 00F8、9 / 00F9|COM_PLUSE_ON/OFF|0003|
|10 / 00FA|COM_FAULT_CLEAR|0001|
|11 / 00FB|PORT_SWITCH_EN|0003|
|12 / 00FC|PORT_NAME|000F|
|13 / 00FD|PORT_SENSORTYPE|00FF|
|14 / 00FE|PORT_GRIDTYPE|003F|
|15 / 00FF|Reserved_3|0000|

`PORT_SENSORTYPE`和`PORT_GRIDTYPE`兼容接收描述信息，不自动修改PT/CT采样比例、PLL、控制模式或保护判据。其他端口、PQ/下垂/谐波等有效但尚未实现的功能仍返回7；这类请求不能显示为控制成功。STOP/OFF优先规则、许可互斥、活动故障和参数存储忙等既有约束保留。

原始`PORT_Control_DATA_Buf`供调试保留接收值；本函数不清零、不回写共享控制块，避免与ARM下一笔写入竞争。没有增加上电清除共享命令块或修改邮箱握手流程。

## 3. 原43字遥调块修改

生产文件：`Source/04_App/10_Host/SstDpramParameter.c`，函数`SstDpramParameter_DecodeAndApplyLegacyBlock()`。

|word / Modbus|字段|接收规则|
|---|---|---|
|0 / 01FF|COM_SYS_MODE|先取00FF；本机HVAC仅低两位，其他有效端口位非零仍返回7|
|1 / 0200|AD_PROT_EN|仅取F000；低12位忽略|
|2..4 / 0201..0203|FO_1/2/3_EN|仅取7FFF；bit15忽略|
|5 / 0204|FB_EN|FFFF；bit15对应U17，是有效位|
|23..26 / 0216..0219|Reserved_1[0..3]|整字忽略，实际回读0|
|其余工程量、PI、EEPROM扩展字段|完整16位数值|保持原符号、缩放及范围校验；有效未实现项非零仍返回7|

先提取位图有效内容，再与实际值比较、构建完整候选。只有通过整组校验的变化项才写入控制参数或FPGA使能；仅保留位改变不会触发多余的FPGA写入。Udc、频率、负PI等完整16位数值不进行掩码截断，保留未修改浮点PI的原精度。

新增57项补充参数事务区不属于Word原43字块，其自身的范围/保留位校验保持原定义。

## 4. 文件及点表

- `SstDpramCommand.c`：控制块有效掩码、RAM副本与本机功能检查。
- `SstDpramParameter.c`：原遥调块位图掩码及保留字忽略。
- `SstDpramProtocol.h`：同步接口注释。
- `tests/dpram_screen.c`：现场原值、保留内容及有效功能回归。
- `tools/dpram_screen_points.py`：点表生成说明同步。
- `README.md`、`DEBUG_STEPS.md`、`DPRAM_MODBUS_POINTS.csv`：接收规则、测试步骤及源码行号同步。

点表仍为2400行，Modbus/DSP地址、类型、倍率及协议版本未调整。10kHz ISR、ADC采样、68个原控制实体和保护配置通过既有源码保留检查；未改官方EMIFA驱动。前次自定义板级WAIT1极性修复和用户已有TaskSlice/launch改动保留。

## 5. 软件验证和编译

加入现场用例后，旧解析器首先在“word0非零、启动字2385”的受理断言失败，复现结果7。修复后以下测试全部通过：

|命令|验证结果|
|---|---|
|`python tests/dpram_screen.py`|单相、三相、仿真器+DPRAM、DPRAM关闭共4配置通过；启用远控配置覆盖2385/49409、全部保留位置1、描述字段、许可互斥、故障/停止优先、遥调掩码、FB bit15、负PI及整组拒绝|
|`python tests/dpram_commands.py`|4种命令所有权配置通过|
|`python tests/dpram_switch_maintenance.py`|6种有效相数/输出/反馈组合通过；2种非法宏组合按预期拒绝编译|
|`python tests/dpram_screen_layout.py`|68个原控制实体、ISR/ADC/保护保留及2400点地址检查通过；运行时设置`PYTHONPATH=build/test_dependencies`|
|`python tools/ccs_validate.py`|独立临时CCS workspace导入及Debug/Release清理后完整编译均退出0；各17条既有警告，无新增警告|

编译日志：`build/ccs_validation/Debug.log`、`Release.log`、`result.json`。固件源输入SHA256：`1fa2911d3228698e8064ddf0f2d6fe8e70a7f57ae780ee37abfb167cd5aa99b9`；编译期间输入一致。

|配置|输出|本次生成文件SHA256|
|---|---|---|
|Debug|`Debug/SST_DpramCompat.out`|`fc52293fe04292a0c600f3b3b7575a6c288a2cf006dbc866bf898bd675e72127`|
|Release|`Release/SST_DpramCompat.out`|`03bd718715df9f79cece60843c906ff23f3b85583c869205057dbe2e5e646f3a`|

## 6. 现场验证

1. 在CCS加载本工程新生成的`.out`，复位后Run；本版DPRAM=1、SIMULATOR=0。保留前次WAIT1板级修复。
2. 先无主回路验证。构造16字控制块，除本笔HVAC启动外其他有效动作清0；word2可直接使用现场49409（C101），word0可用非零保留内容。完整写入00F0..00FF后，最后写1FFE=1。
3. 观察1984控制计数递增；无活动故障且存储空闲时1985应为0、1988应为FFFF。原始COM_START_PORT16仍可为49409；在后台解析函数中看`command_words[2]`应为1。
4. 当前独立DAB版本通过START入口后只把`g_chb_dab_command.run_enable`置1，`dab_enable`保持原值；DAB必须另发00F8 bit0=1。由核心状态1989、故障198A、实际PWM位198F判断启动进程；受理成功不等于无输入、采样或预充条件也可以进入RUNNING。只在无功率条件下于后台入口/返回处单步，不在发波时暂停ISR。
5. 若1985仍为7，立即读1988定位具体字号并查看该字掩码内的值。此时应是有效但本机未实现的动作；不能通过屏蔽有效位或取消故障联锁强行启动。结果4表示忙，5表示拒绝；后续参数操作可能覆盖共享error_word，应在本笔返回后立即观察。
6. 遥调先读取43字实际底稿，仅置本记录第3节的保留位/字，再完整写回及提交邮箱2。停机且存储空闲时1987应为0，回读保留内容为0，实际Udc/PI/有效保护使能保持原值。
7. 再分别验证STOP、CHB/DAB OFF、完全停机后的RESET及正常参数修改。真实功率启动与10kHz GPIO时序仍需现场验证；本次已完成主机回归及CCS编译，未连接硬件执行启动。
