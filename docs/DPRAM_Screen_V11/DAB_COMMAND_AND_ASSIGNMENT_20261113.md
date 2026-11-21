# 屏幕控制、独立DAB及遥调赋值说明

日期：2026-11-13。工程：`D:\Code\SST\AnyEyesVersion_DpramCompat`。

本文构建后，控制块又调整为未接入位只忽略、不连带拒绝正常命令；最新解析语义及构建见 [CONTROL_FIELD_IGNORE_20261113.md](CONTROL_FIELD_IGNORE_20261113.md)。本文第6节指纹为前一次独立DAB交付映像。2026-11-17原遥调已改为直接赋值，下方第3/5节同步更新；本次验证见 [PARAMETER_SIMPLIFICATION_20261117.md](PARAMETER_SIMPLIFICATION_20261117.md)。

本次在掩码修复版本上解除HVAC START对DAB的启动联动，并沿实际收发及赋值路径补充源码注释。以下语义适用于Word原16字控制块、原43字遥调块；AnyEyes/VOFA配置、仿真器权限及原控制状态机保持原定义。

## 1. 当前启停对应关系

每笔控制仍完整写16字到Modbus `00F0..00FF`，最后写`1FFE=1`；只有协议有效位参与解析。

|屏幕字段|Modbus / 位|统一命令中的实际赋值|另一请求|
|---|---|---|---|
|COM_START_PORT.HVAC_Start|00F2 bit0|`run_enable=1`|`dab_enable`保持原值|
|COM_PLUSE_ON DAB|00F8 bit0|`dab_enable=1`|`run_enable`保持原值|
|COM_PLUSE_OFF DAB|00F9 bit0|`dab_enable=0`|`run_enable`保持原值|
|COM_PLUSE_ON CHB|00F8 bit1|`run_enable=1`|`dab_enable`保持原值|
|COM_PLUSE_OFF CHB|00F9 bit1|`run_enable=0`|DAB请求保持；闭环实际PWM仍受核心停机约束|
|COM_STOP_PORT.HVAC_Stop|00F3 bit0|两路请求均清0|保留原整机STOP语义|
|COM_FAULT_CLEAR|00FA bit0|完全停机时`fault_reset=1`|不置1任何使能|

从两路请求均为0开始，仅发HVAC START后应为`run_enable=1/dab_enable=0`。单独DAB ON后才为`1/1`；DAB OFF后回到`1/0`。重复HVAC START不会重新打开DAB，也不会把已单独打开的DAB清掉。

整机STOP、急停及实际故障仍撤销两路请求。OFF保持既有优先规则：原16字解析入口遇到有效OFF就只消费OFF，其余START/ON应另起一笔发送。闭环下DAB请求独立，但实际DAB发波仍需原状态机进入RUNNING并通过门控；DAB ON不能跳过预充、旁路和PLL。开环沿用原核心的独立CHB/DAB测试能力。

## 2. 在代码中按顺序查找控制赋值

```text
GPIO101通知标志
  -> TaskSlice_Service
  -> SstDpramHost_ProcessIncomingBlocks
  -> DPRAM_Read：读左邮箱，复制共享块到RAM
  -> SstDpramCommand_ProcessControlBlock：按有效掩码提取，检查能力及联锁
  -> SstControlCommand_ApplyRequests：实际写run_enable/dab_enable/fault_reset
  -> ChbDabControl_Service / FastStep：依据请求和原状态机决定实际PWM
```

- `Source/02_Peripheral/01_DPRAM/DPRAM.C`的`DPRAM_Read()`：注释写明邮箱1/2、Modbus/DSP地址及复制目标。这里仅拷贝，不执行旧`DPRAM_ReadDataDispose()`算法。
- `Source/04_App/10_Host/SstDpramHost.c`：在“接收第1步、第2步A/B、第3步”标出原始缓存、调用对象、结果及回读位置。
- `Source/04_App/10_Host/SstDpramCommand.c`：第1步原始16字提取；第2步STOP；第3步OFF；第4步能力/联锁；第5步五实参提交。末尾逐项说明word2/8/10对应哪个命令成员。
- `Source/04_App/04_Control/SstCommand.c`的`SstControlCommand_ApplyRequests()`：第4步直接写`run_enable=1`；第5步DAB ON直接写`dab_enable=1`；第3步DAB OFF直接清`dab_enable`。不再调用会把两路同时置1的旧`ChbDabControl_ApplyRemoteCommand()`。

请求变量和实际PWM不同：`run_enable/dab_enable`是操作请求，`g_chb_dab_status.ac_pwm_enabled/dab_pwm_enabled`才是实际使能镜像。屏幕`1985=0`只表示本笔被受理，实际PWM读`198F`的bit0/1。

## 3. 遥调传输到最终成员

屏幕先读实际43字底稿，只改目标字段，完整写`01FF..0229`，最后写`1FFE=2`。`DPRAM_Read()`把原始编码复制到`PORT_Regulate_DATA_Buf.DPRAM_Block2Buf`。

2026-11-17已取消43转57，当前路径如下：

```text
SstDpramParameter_DecodeAndApplyLegacyBlock(words[43])
  -> 检查权限、完全停机及EEPROM空闲
  -> 检查本43字字段，按协议掩码忽略保留位/保留字
  -> 在本函数内直接赋值实际参数，PI按int16解码再除比例
  -> Host回读实际43字，DPRAM_Write(20)回传
```

|原word / Modbus|最终成员及转换|
|---|---|
|0 / 01FF|`PortRunFLAG.Bbits.Port_RunOrTest`取bit0；`Port_AutoOrManual`取bit1|
|1 / 0200|取F000写`Port_AD_PORT_EN`，变化后调用`Port_ADProtect_EN()`|
|2 / 0201|取7FFF写`Port_FO_1_EN`|
|3 / 0202|取7FFF写`Port_FO_2_EN`|
|4 / 0203|取7FFF写`Port_FO_3_EN`；三组任一变化后调用`Port_FO_EN_Set()`|
|5 / 0204|写`Port_FB_EN`，变化后调用`Port_FB_EN_Set()`|
|8 / 0207|直接写`g_chb_dab_command.dc_voltage_ref_v`，1V/count|
|9 / 0208|除10得到Hz；开环写`open_loop_frequency_hz`，闭环写`pll_nominal_frequency_hz`|
|27 / 021A|int16二补码除100000，写`g_chb_dab_params.voltage_kp`|
|28 / 021B|int16二补码除100，写`g_chb_dab_params.voltage_ki`|
|29 / 021C|int16二补码除1000，写`g_chb_dab_params.current_kp`，dq共用|
|30 / 021D|int16二补码除1000，写`g_chb_dab_params.current_ki`，dq共用|
|23..26 / 0216..0219|保留字忽略，回读0|
|6/7、10..22、31..42|有效但未实现的PQ/谐波/下垂/组2、3PI/EEPROM扩展；非零返回7|

负PI示例：word29=`FB1E`按int16为-1250，写入`current_kp=-1.25`；word30=`C180`为-16000，写入`current_ki=-16.0`。Ki仍是连续积分增益，PI执行时乘一次采样周期。只改Udc时未改PI保持原float精度。

原遥调只检查本块，不再检查无关启动/DAB/原软件保护参数；检查通过才直接赋值。系统方式不是启动使能，原遥调不写`run_enable/dab_enable/fault_reset/ac_mode`。57项扩展区独立保留，原软件保护和预充巡检阈值仍从扩展区设置，详见`PARAMETERS.md`。

## 4. 实际值回读许可修复

旧`DPRAM_Write(WriteBlock_remRegulate=20)`仅在`FLAG_RemRegulate_SetOver=1`时复制43字、通知右邮箱20，并消费标志清0。旧控制初始化会置1一次，但新Host初始化及后续遥调路径此前没有逐笔重置，导致上电一次回写后，后续实际参数回读可能不执行。

当前在`SstDpramHost_Initialize()`及每笔邮箱2处理后，先调用`SstDpramParameter_EncodeLegacyBlock()`生成实际值，再明确置`FLAG_RemRegulate_SetOver=1`，随后调用`DPRAM_Write(20)`。成功、越界、忙、不支持或拒绝都回读实际值；候选失败不会被回显为成功。`1986`为消费计数、`1987`为受理结果，邮箱20仅代表实际值回写通知。

## 5. 调试顺序

仅在无主回路时停断点。每次命令从全零16字构造本笔动作，避免残留有效STOP/OFF。

1. 在Host的`DPRAM_Read()`后确认`FLAG_NEW_L`、原始控制/遥调缓冲内容。
2. 启动：在`SstDpramCommand_ProcessControlBlock()`看`command_words[2]`，再进入`SstControlCommand_ApplyRequests()`的START赋值。初始两请求0时，仅START应得到1/0。
3. DAB：另发word8=1，应为1/1；再发word9=1，应为1/0；重复START仍为1/0。发word3=1应全部清0。
4. 遥调Udc：先读43字，在停机、两路PWM关闭时修改word8。进入`SstDpramParameter_DecodeAndApplyLegacyBlock()`看`words[8]`，在第5步直接赋值语句单步核对`dc_voltage_ref_v`。
5. 负PI：按表核对`words[29/30]`，在同函数第6步直接赋值语句单步核对`current_kp/ki`；只改Udc时其他PI不应重新量化。
6. 回传：Host重建实际43字后，确认回写前ready标志=1；`DPRAM_Write(20)`后ready=0、原01FF..0229为实际值。连续提交至少两笔合法修改，再提交一笔非法候选，验证失败后回读仍是上一笔实际值。
7. 结果按控制1984/1985、遥调1986/1987及错误1988判断；状态、故障、实际PWM分别读1989、198A、198F。运行中采用连续观察，不暂停10kHz中断。

## 6. 验证及生成文件

|验证|结果|
|---|---|
|`tests/dpram_commands.py`|4种所有权配置通过；HVAC START不联动DAB、重复START保持DAB状态、STOP/OFF及存储锁通过|
|`tests/dpram_screen.py`|单相、三相、仿真器+DPRAM、DPRAM关闭4配置通过；实际16字入口验证START/DAB ON/OFF、保留位、负PI及参数整组拒绝|
|`tests/dpram_maintenance.py`|4种配置通过；共享统一命令入口的旧邮箱21也按新START语义执行，重复事务仍不会重复赋值|
|`tests/dpram_host_reply.py`|实际Host初始化与连续6笔成功/失败回传全部建立ready许可并回读实际值；DPRAM关闭时无访问|
|`tests/dpram_screen_layout.py`|68个原控制实体、ISR/ADC/保护保留及2400点地址检查通过|
|`tools/ccs_validate.py`|独立临时workspace导入、Debug/Release清理后完整编译通过；各17条既有警告，无新增警告|

固件源输入SHA256：`454509edfcd21a9edb38c98d667ba740ab53190967088083cc6fba8b97f615ca`。编译期间源码一致，结束后再次核对当前源码与构建输入一致。

|配置|生成文件|SHA256|
|---|---|---|
|Debug|`Debug/SST_DpramCompat.out`|`564029f4a4f837911c6140926d3eef180560c738eac5e29adcbf981438baa3ef`|
|Release|`Release/SST_DpramCompat.out`|`19e3c6ce0a1db1ae1cc3c3d357229c379eb62dda108372933e2a7a5a73667679`|

日志：`build/ccs_validation/Debug.log`、`Release.log`、`result.json`。官方EMIFA库、原10kHz控制路径及用户已有TaskSlice/launch改动保留。现场ARM、屏幕、真实PWM及时序尚未连接验证，以上为主机回归、源码核对和CCS编译结果。
