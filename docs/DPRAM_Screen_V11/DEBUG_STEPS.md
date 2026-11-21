# ARM与Modbus屏幕联调步骤

> 2026-11-18：QF1合/分、KT1保持及预充反馈验收见 [S301调试步骤](../SWITCH_301_QF1_KT1_20261118.md#6-调试与验收)。DOUT60000240、DIN600003C0；不要沿用旧通道及旧极性。


先用无主回路控制系统验证通信。调试断点会暂停控制；带实际PWM时采用连续观察和GPIO测量，不在10kHz ISR内停断点。

## 1. 导入和核对映像

1. CCS当前workspace导入 `D:\Code\SST\AnyEyesVersion_DpramCompat`，工程名应为 `AnyEyesVersion_DpramCompat`，不与V1同名。使用新launch，不沿用原工程的加载路径。
2. 默认Debug或Release都输出本工程 `SST_DpramCompat.out`；确认Expressions的宏对应DPRAM=1、SIMULATOR=0、BENCH_INPUT_SIM=0。测试宏参数必须重新编译，不能在Expressions中改编译宏。
3. 新工程保留真实采样和现场参数。先保持run_enable/dab_enable=0；输出/反馈宏默认0，手动开关应拒绝，不能把这当通信失败。
4. ARM使用已配置IP的Modbus TCP 502端口。03读寄存器；06写单字；10十六进制写多字。不要向ARM另发A5 5A串口帧。

## 2. 最小物理链路验证

1. 屏幕/Modbus工具读取寄存器1981、1982、1983，期望 `5353/0102/0039`（参数数57；0102为无需序号的操作字接口）。
2. 读1980应为偶数且随遥测发布变化；读198C/198D的10ms计数应递增。
3. CCS Memory Browser选16-bit Hex，DSP地址 `0x66003300`，核对与ARM寄存器区同字。特别核实Modbus软件是0起始还是40001显示编号。
4. 停机且1B01=0、1B04=0时，只写4到1B01；等1B01=0且1B04=0，确认1B03=0。再比较1A00候选和1A80实际前57字。1B00无需设置，1B02为DSP自动维护的完成计数。

若03/06正常但上述魔数不对，优先核对下载映像、DPRAM宏、FSMC地址步长和16位字节序。不要通过向只读魔数区写5353来伪造连接正常。

## 3. 旧块2：电压和负PI

1. 先读01FF..0229共43字，保存底稿。读1986保存遥调计数。
2. 将底稿word8写成现场需要的**单模块目标**；例如60V写60，不是600。其余字段保留实际底稿，不用全零块替代。
3. 可先在无主回路条件将word29/30写为 `FB1E/C180`，即Kp=-1.250、Ki=-16.000，验证有符号路径；真正运行前恢复经确认的控制参数。
4. 10写完整43字到01FF，之后**单独最后06写1FFE=2**。不要先写邮箱、再补参数。
5. 等1986计数变化，读1987期望0。再读021C/021D和1A98/1A99，应为相同二补码；CCS看current_kp/current_ki应为负值。
6. 修改一个Udc后，CCS比较其他PI浮点位模式应保持原值；不得从-1.249166667无故变成-1.249。
7. 拒绝测试：给PI组2字段021E写非零，再提交邮箱2。应result=7、Udc/所有PI均不变化、旧块回读恢复实际值。该试验后重新读底稿。

建议断点（都在后台）：`SstDpramHost_ProcessIncomingBlocks` 的 `DPRAM_Read` 后；`SstDpramParameter_DecodeAndApplyLegacyBlock`第5/6步直接赋值处。57项扩展事务才进入`SstDpramParameter_ApplyParameterSnapshot`和`SstParameterTable_SnapshotApplyChanged`。Expressions同时观察输入数组、result、g_chb_dab_params和g_chb_dab_command；不能只看PORT_Regulate_DATA_Buf。

模块故障位置新增只读点`051E/051F/0520`，分别为A/B/C相701的物理输入位图；读取、掩码、UNIT位置绑定及故障复位验证见 [FO_MODULE_FAULT_BITMAP_20261117.md](FO_MODULE_FAULT_BITMAP_20261117.md)。这些是遥测，不需要写遥调邮箱。

## 4. 直接参数：预充、保护和运行限幅

所有参数都按 [PARAMETERS.md](PARAMETERS.md) 查序号，待应用地址=1A00+序号、实际地址=1A80+序号。

|例子|待应用|实际|编码示例|
|---|---|---|---|
|预充门槛V|1A02|1A82|25V→250|
|预充稳定拍数|1A03|1A83|20→200ms|
|预充超时|1A04|1A84|1000→10s|
|电压外环输出/运行Id限幅|1A11|1A91|50A峰值→5000；启动还有原动态限幅|
|启动Id目标|1A15|1A95|1A→100，支持负数|
|current_kp|1A18|1A98|-1.250→FB1E|
|current_ki|1A19|1A99|-16.000→C180|
|快速瞬时OC|1A1E|1A9E|60A→6000|
|快速均压OV|1A1F|1A9F|80V→800|
|快速确认拍数|1A20|1AA0|5→500us连续超限|
|原RMS一级OC/时间|1A2A/1A2B|1AAA/1AAB|A/10ms计数，时间判据严格大于|
|原RMS二级OC/时间|1A2C/1A2D|1AAC/1AAD|同上|
|原均压OV/时间|1A2E/1A2F|1AAE/1AAF|V/10ms计数|
|巡检预充低压/高压|1A30/1A31|1AB0/1AB1|V×10；与快速均压OV是不同路径|

步骤：先单写1B01=4刷新候选并等完成 → 改待应用字 → 最后单写1B01=1应用 → 等op=0且busy=0，确认result=0 → 读实际区。全程不用1B00。只写待应用区应完全不影响活动值；运行时APPLY应返回4且实际值不变。改硬件采样周期应返回2，不改变10kHz。

这些编码例子用于证明通信和比例，不能当成适合任意主回路的整定值。限流、保护阈值和电压目标需共同按实际试验配置。

## 5. 保存/加载

2026-11-19已适配DSP侧BL24C512A，硬件接线和初始化恢复说明见[EEPROM与光纤配置](../EEPROM_BL24C512A_ADAPTATION_20261119.md)。

1. 停机、两路PWM都关闭、busy=0。
2. 参数已经生效并核对实际值后，只写1B01=2；不用写1B00。原43字遥调成功后可直接保存。
3. 操作字被DSP清0，busy=1表示尚在保存；v3名义约2.8秒后，等1B01=0且1B04=0，再检查1B03=0才是成功。1B02由DSP自动递增，无需请求序号。快速失败时busy可能来不及在屏幕上显示，以最终结果为准。
4. 修改RAM参数并APPLY；只写1B01=3加载，应恢复已保存的PI、保护和预充值。v3还恢复系统管理方式、AD/FO/FB及六组501接线位图，并重算模块数；启停请求和临时快速保护旁路不恢复。LOAD后用1B01=4刷新候选再编辑。
5. 未知旧EEPROM或CRC失败返回6且不部分应用；不要把该错误改显示为“已恢复默认”。上电恢复取决于CHB_LOAD_EEPROM_PARAMETERS，当前源码为1，本次未修改；断电测试需先确认保存确实成功。
6. 无主回路可在EEPROM假硬件测试中验证写失败；真实EEPROM失败时保留result、版本和原参数，排查I2C硬件，不反复自动START。
7. 结果6时，等op=0且busy=0后再写1B01=2，应重新尝试保存；不再要求换序号。SAVE忙时再次按保存/加载，操作字会清0丢弃，不排队、不重启当前保存。

## 6. 启停、脉冲与复位

旧控制块示例：全零16字，word2=1启动；word3=1停止；word8=2为CHB ON；word9=2为CHB OFF；word10=1复位。每次**完整写16字到00F0，最后06写1FFE=1**。

2026-11-13掩码修复及独立DAB验证：无主回路且无活动故障、初始两请求为0时，可以令word0为非零保留内容、word2为现场原值49409（C101），其他有效动作位清0，再提交邮箱1。期望1984递增、1985=0、1988=FFFF；CCS中原始COM_START_PORT16仍为49409，SstDpramCommand_ProcessControlBlock内command_words[2]应为1，进入统一命令入口后run_enable=1、dab_enable仍为0。另起全零16字、只置word8=1提交后，DAB请求才置1；只置word9=1提交后，DAB请求清0、run_enable保持1。原始块保留位不必由屏幕额外清零才能启动；有效STOP/OFF仍有优先权。控制块未接入位不再导致整包结果7。

未接入字段共存验证：初始两路请求0，发送word2=1、word4=2，其他本机有效动作清0。期望1985=0、run_enable=1、dab_enable=0、只读19A5=0010（word4含忽略位）。未接入字段只记录、不动作；若本机请求仍不受理，检查结果4的忙、5的故障/权限、3的许可互斥，以及本机有效STOP/OFF，不再从未接入模式位找拒绝原因。

遥调掩码验证：先读43字底稿；只置word0的高8位、word1的低12位、word2..4的bit15及word23..26保留字，再提交邮箱2。停机且存储空闲时应1987=0，回读这些保留内容为0，活动Udc/PI/保护使能不变。修改有效保护使能位才会写入对应FPGA接口；负PI仍按int16二补码解码。有效的其他端口方式位及未实现参数非零仍返回7。

1. 保存1984计数，发送STOP；等计数变化，run_enable/dab_enable应为0。
2. 无主回路/输入条件不成立时发送START，期望“请求被受理”并进入对应状态/超时故障；不能要求缺采样也进入闭环RUNNING。
3. 实际闭环按STOPPED→PRECHARGE→BYPASS_DELAY→WAIT_PLL→RUNNING推进。观察1989核心状态、0500 Word状态和198F PWM实际标志。
4. 重复START不应把启动计时、PI历史或状态反复清零。STOP必须可在保存忙期间撤销请求。
5. 故障后先STOP、确认两路实际PWM均关闭，再单独RESET；存在活动硬件故障时允许重新上报，不能强制解锁。
6. 手动开关先切管理方式为手动、发许可，再分笔合进线/旁路；宏0期望拒绝5。实际输出电平、反馈极性和接线确认后才编译宏1进行该项验证。

状态观察：1989核心状态；198A核心故障；198B启动阶段；198F位图；1994/1995模块Udc float；1996/1997 Idref float；1998/1999 Id反馈；199A/199B Iq；199C/199D启动电压给定。读取跨字量按epoch做一致性检查。

源码断点顺序（仅无主回路时使用）：`DPRAM_Read()`复制后看两个原始块；`SstDpramCommand_ProcessControlBlock()`提取后看`command_words`；进入`SstControlCommand_ApplyRequests()`对应bit的赋值语句，看run_enable/dab_enable分别变化。遥调从`SstDpramParameter_DecodeAndApplyLegacyBlock()`看旧word、value及candidate；进入`SstDpramParameter_ApplyParameterSnapshot()`确认整组通过；到`SstParameterTable_SnapshotApplyChanged()`和`WriteEntry()`看最终地址、raw/scale及float。最后回Host确认`FLAG_RemRegulate_SetOver=1`后调用`DPRAM_Write(20)`，回写完成后标志应被清0。

## 7. 故障反馈核对

无主回路且完全停机时，可在CCS临时修改锁存输入并立即连续读取：

|输入|0502应有的位|补充诊断|
|---|---|---|
|内部Port_RealFault=2000|bit12 OV及bit15；不得显示通信|19C1保留2000原字|
|内部Port_RealFault=4000|bit10 OC及bit15；不得显示同步|19C1保留4000原字|
|FaultType bit12通信|bit13及bit15|详细码39103|
|FaultType bit11同步|bit14及bit15|详细码39104|
|核心快速OC原因3|bit10及bit15|核心原因3，详细29103|
|核心快速OV原因4|bit12及bit15|核心原因4，详细29104|
|非法配置原因6|bit15|详细29105，不伪报硬件OC/OV|

后台原故障生产者可能重新覆盖调试写入的字，推荐在生产者后设条件断点核对一次，或使用主机测试。**不要在带主回路时往故障字写0。**实际故障需核对原始FaultType、Port原字、软件OC/OV锁存和详细码，不能只凭屏幕名称归因。

## 8. 10kHz时序实测及验收记录

1. 使用既有ISR首尾GPIO43/45，记录相同采样、PWM、缓存和FPGA条件下基线与新工程的典型/最大高电平宽度及周期抖动。
2. 分别测停机、闭环稳态、连续Modbus读、合法参数拒绝、SAVE过程；带PWM时不要打停ISR。
3. 检查中断周期仍100us、无遗漏；同时记录ARM命令→DSP计数变化→实际状态的延迟。主机编译/测试结果不能替代该记录。
4. 将屏幕软件“每控件地址、类型、倍率、字序、是否最后写邮箱/操作字”逐项与CSV对齐。旧块完整收发、STOP、负PI、失败拒绝、SAVE ACK、故障详情都通过后再进行功率回路试验。
5. 栈大小沿用原工程0x800字节。无主回路时按.map确认栈段边界，在启动前填充未使用栈区的水印，运行上述最大通信/保存场景后检查剩余水印；不要在运行中覆盖当前栈。记录最大用量和余量，本次尚无上板水印结果。

本次已完成软件与构建验证；真实ARM/FSMC、屏幕控件及GPIO时序待现场填写。当前工作区历史控制断言已有继承失败，详见修改记录。
