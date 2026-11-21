# UNIT、701、主控与屏幕故障链路核查

核查日期：2026-11-19。DSP工程：D:/Code/SST/AnyEyesVersion_DpramCompat。
现场701已确认使用D:/Code/FPGA/Pango/701；UNIT按D:/Code/FPGA/Pango/UnitBoard_DAB_Test_NoInterlock核查。
本次仅分析并增加本文档，未修改DSP/FPGA源码，未下载或烧录设备。

后续更新：同日已单独完成[BL24C512A适配](EEPROM_BL24C512A_ADAPTATION_20261119.md)，修复无校验旧EEPROM启动读取及保护使能覆盖问题，v3统一保存接线/使能。本文保留核查时事实；701输入极性、GPIO142及详细故障分类不属于此次EEPROM修改范围。

## 1. 结论与当前缺口

UNIT存在独立的硬件故障光纤输出OP_FO，接701；详细故障状态则经OP_TX串行发送给501。两条光纤承担不同职责。

|环节|当前实现|核查结果|
|---|---|---|
|UNIT就地保护|最终门极与fault_svg/fault_dab及shutdown相与|两个测试旁路参数均为0，本地保护参与门极封锁|
|701接收|16路均使用.INPUT(~OP_IN[n])|保留旧板施密特反相后的极性，不适合本次核对的新板直接接收电路|
|主控汇总|FB经FB_EN筛选后锁存，产生DSP_INT_FAULT|故障汇总不会直接并入SHUTDOWN，统一封波还依赖DSP|
|DSP故障引脚|GPIO142设为GPIO_DIR_OUTPUT|该引脚由FPGA驱动，应改为输入，当前配置存在驱动冲突及中断可靠性问题|
|DSP模块原因分类|先FaultDispose，再立即ForceFault|现代FAULT把Port_Stop置1，后续旧分类计数被清零，501原因解码被截断|
|屏幕模块位置|051E/051F/0520读取三块701的FO_LATCH|已有遥测点，但实际位置准确性仍依赖701极性、使能及物理接线|

因此，不能把“UNIT本地已封波”直接等同于“当前主控整机停机链路已验证可靠”。

## 2. UNIT本地封波与两路故障输出

UNIT的TEST_BYPASS_EXTERNAL_INTERLOCKS与TEST_BYPASS_SVG_EXTERNAL_INTERLOCKS均为0。

- CHB门极：PWM_H1/H2与~fault_svg、~shutdown_svg相与。
- DAB门极：PWM_H3/H4与~fault_dab、~shutdown_dab相与。
- 公共OV、OC、OT参与两个桥的故障；CHB驱动故障参与CHB封波，DAB驱动故障参与DAB封波。
- 外部SHUTDOWN参与两个桥的封波。
- OP_FO = fault_svg | fault_dab，正常0，故障1；独立UV位和外部SHUTDOWN位未直接并入该汇总。

程序位置：[UNIT最终门极与OP_FO](/D:/Code/FPGA/Pango/UnitBoard_DAB_Test_NoInterlock/source/main.v:660)、[故障锁存及汇总](/D:/Code/FPGA/Pango/UnitBoard_DAB_Test_NoInterlock/source/main.v:708)。

详细状态帧由[UNIT状态发送](/D:/Code/FPGA/Pango/UnitBoard_DAB_Test_NoInterlock/source/main.v:1061)构造：

|状态位|含义|
|---|---|
|bit0|直流过压实时输入|
|bit1|IG过流实时输入|
|bit2|IR过流实时输入|
|bit3|过温实时输入|
|bit4|CHB/SVG驱动实时故障|
|bit5|DAB驱动实时故障|
|bit6|欠压实时输入|
|bit7|外部SHUTDOWN|
|bit8..13|依次为直流过压、IG过流、IR过流、过温、CHB驱动、DAB驱动锁存|

501识别帧头后，将低14位保存到各模块STATUS寄存器；见[501接收解码](/D:/Code/FPGA/Pango/501_compatible_build/source/main501.v:834)。701不解析这类串行帧，也不需要复制UART接收器或修改波特率。

## 3. 701接收极性与锁存语义

清华原701源码与当前Pango工程的保护主体一致，迁移主要涉及时钟IP、LED和注释；16路输入仍取反，见[当前701接收](/D:/Code/FPGA/Pango/701/source/main701.v:158)。

新版701图纸的接收头为AFBR-2529Z，Data_OUT经33Ω串阻直接连FPGA，没有旧注释描述的施密特反相器。UNIT的SHUTDOWN输入已改为直接采样；501与UNIT的async_receiver也已经取消~RxD。

按该硬件链路，应将701的16处.INPUT(~OP_IN[n])改为.INPUT(OP_IN[n])，同时改正旧极性注释。对应关系为：

|情况|UNIT OP_FO/发射控制|光纤|701 OP_IN原始输入|应解释的故障|
|---|---|---|---|---|
|正常|0|亮|0|0|
|UNIT故障|1|灭|1|1|
|故障光纤断开|—|无光|1|1|

实际烧录后应测量正常/故障两种输入电平确认上述关系。本文的仿真只验证RTL逻辑，不代替实物光电通路验证。

701中reg_FO = fo & reg_FO_EN。FB=fo_block，汇总故障自保持。FO_LATCH在fo_block首次置位时冻结，之后不持续OR累加；见[701汇总与锁存](/D:/Code/FPGA/Pango/701/source/main701.v:283)。后续其他通道故障可能出现在实时FO中，但不进入已有FO_LATCH。读取位置位图时必须按“首次锁存现场”理解。

FPGA上电默认FO_EN=FFFF，滤波器输出初始为1，调试时需在通道配置完成且输入稳定后执行完整硬件复位。未消除的真实故障不会被复位清掉。

## 4. 主控到DSP，再到整端口封波

链路为：

    UNIT OP_FO -> 701 OP_IN[n] -> FO_EN -> FO_LATCH、FB
      -> 主控对应FB输入 -> FB_EN -> FB_LATCH、DSP_INT_FAULT
      -> DSP GPIO8[13]（库编号142）上升沿 -> FAULTIsr
      -> FaultCheckAndDispose_StepOne -> PWMPluse_Disable
      -> 主控DSP_CTL.SHUTDOWN=1及501模块PWM使能撤销
      -> 601关断光纤 -> UNIT SHUTDOWN -> 门极封锁

[主控故障输出](/D:/Code/FPGA/Pango/MainControl/source/main403.v:1519)来自fb_block；但[主控SHUTDOWN](/D:/Code/FPGA/Pango/MainControl/source/main403.v:1538)只来自reg_DSP_CTL[0]，未直接OR入fb_block。601_openlooptest中OP_OUT为reg_SHUTDOWN与总shut的按位或；现场601位流未在本轮单独确认。

[FAULTIsr](/D:/Code/SST/AnyEyesVersion_DpramCompat/Source/03_System/02_ISR/Redef_Interrupt.c:533)读取硬件锁存、封波，再调用ChbDabControl_ForceFault(CHB_FAULT_HARDWARE)。核心故障编号为5，run_enable、dab_enable清零并进入FAULT。闭环FAULT同时释放KT1、发QF1分闸命令；开环FAULT保留手动开关管理，但仍封PWM。

[GPIO142方向](/D:/Code/SST/AnyEyesVersion_DpramCompat/Source/03_System/00_Board/SstBoard.c:148)目前为OUTPUT，应改INPUT。此处不能仅凭“有上升沿中断配置”认定输入通路已经正确，也不能笼统断言输出模式绝不可能检测到中断。

详细原因分类存在确定的流程冲突：

1. [故障第一阶段](/D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/08_Fault/FaultCheck.C:564)把ModuleFault_DisposeTimes置3。
2. 本次FaultDispose先封波，将3减为2；501原因读取在后续分支。
3. 同次FAULTIsr立即ForceFault，FAULT入口将Port_Stop置1。
4. 后续[FaultDispose入口](/D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/08_Fault/FaultCheck.C:699)见Port_Stop==1，将计数清0，旧原因解码无法继续。

此外，第一阶段Port_BKU_Send(1)会冻结501的状态更新；快速FO先到、串行原因帧后到时还存在冻结过早的时序问题。修复原因诊断应在立即封波后独立完成，不应为了等串行帧推迟保护动作。

## 5. 屏幕已有上报与模块定位

以下为Modbus十六进制地址，16位无符号只读值：

|屏幕地址|内容|DSP DPRAM字节地址|
|---|---|---|
|0502|按Word语义转换的PORT_FAULT，bit6模块汇总、bit15任一故障|66000A04|
|051E|A相701 FO_LATCH与当前FO_1_EN相与|66000A3C|
|051F|B相701 FO_LATCH与当前FO_2_EN相与|66000A3E|
|0520|C相701 FO_LATCH与当前FO_3_EN相与|66000A40|
|198A|核心CHB_FAULT_*故障枚举，HARDWARE=5|66003314|
|19C4|首个五位分类故障码|66003388|
|19C8..19E7|最多32个当前分类故障码|66003390起|

[位置遥测代码](/D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/10_Host/SstDpramTelemetry.c:173)直接读取各701硬件锁存，不依赖旧FOLatch_*_TAD或当前是否运行；FO整组为0时不访问该板。FB_EN不屏蔽这里的诊断，FO_EN会屏蔽。

bitN对应OP_IN[N]，也就是701图纸RX(N+1)/STATUS(N+1)，不是重新连续编号的UNIT号。例如051E=0004表示A相701的OP_IN[2]/RX3；它是哪块UNIT，取决于故障光纤接线。

五位故障码属于原因分类：19104模块汇总、19105模块过流、19106模块过压、19107过温、19108 CHB驱动、19109 DAB驱动。它们不包含UNIT编号。当前分类链路缺口可能使上报停留在19104或未细分硬件19101。屏幕需要绑定上述位置点才能显示具体通道；DSP发布点位不等于已修改屏幕画面。

## 6. 保护使能默认值、覆盖来源和写入位置

源码初始默认见[DpramDefaults](/D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/10_Host/DpramDefaults.c:21)：

|配置/运行变量|源码默认|作用|
|---|---|---|
|AD_PROT_EN / Port_AD_PORT_EN|7000|202板bit12/13/14，即PROT_C/D/E；bit15 PROT_F关闭|
|FO_1_EN / Port_FO_1_EN|3F7F|A相701，bit0..6及8..13共13路|
|FO_2_EN / Port_FO_2_EN|3F7F|B相701，同上|
|FO_3_EN / Port_FO_3_EN|3F7F|C相701，同上|
|FB_EN / Port_FB_EN|4441|bit0采样板、bit6 A701、bit10 B701、bit14 C701|

因此4441是采样板加三块701，共四路。当前Analog_PluginUnit_3宏实际等于2，IO_PluginUnit_2实际等于3，应按宏值与硬件总线理解，不能只看历史名字的数字后缀。

使能不会随单相宏、在线模块数或PM配置自动裁剪。未安装的701应将对应FO整组设0，实际安装的各通道按光纤接线设置。不要用全开使能代替接线核对。

初始化顺序：Initial_Variable先关闭主控FB使能，再建立DPRAM默认值，ControlDefaults_InitCoefficients调用旧EEPROM页读取，之后RemoteRegulate_Dispose(1)把活动使能写FPGA，最后开放故障中断。

特别注意：当前CHB_LOAD_EEPROM_PARAMETERS默认为1，并连带定义EEPROM_ON。[旧页1读取](/D:/Code/SST/AnyEyesVersion_DpramCompat/Source/02_Peripheral/02_Storage/X1226_RW.C:199)会覆盖上述使能，而且未检查r_eeprom_section_read返回值；若读取失败，会使用未初始化的DataTemp。故源码默认值不代表现场最终活动值。应观察Port_AD_PORT_EN、Port_FO_1/2/3_EN、Port_FB_EN，并读回FPGA寄存器。现代50字存储镜像本身不保存这五项使能，不能与旧EEPROM页读取混为一谈。

运行中屏幕通过43字遥调块设置：0200 AD、0201/0202/0203三组FO、0204 FB，完整提交块后写1FFE=2。解析见[SstDpramParameter](/D:/Code/SST/AnyEyesVersion_DpramCompat/Source/04_App/10_Host/SstDpramParameter.c:128)：AD只取F000，FO只取7FFF，FB保留16位。真正硬件写入由Port_ADProtect_EN、Port_FO_EN_Set、Port_FB_EN_Set完成；扩展参数APPLY也更新同一组变量。

## 7. 本轮验证与现场核验顺序

本轮用Icarus对现有701 RTL及临时取消输入取反的副本分别仿真，时钟/LED IP用替身，未修改工程文件。使能0005时：

|输入场景|现有取反版FO|直接输入版FO / FO_LATCH / FB|
|---|---|---|
|正常raw=0000|0005|0000 / 0000 / 0|
|通道0故障raw=0001|0004|0001 / 0001 / 1|
|后续再故障通道2，raw=0005|0000|0005 / 0001 / 1|
|故障消失并完整复位|仍把正常解释为故障|0000 / 0000 / 0|

两组逻辑断言通过；仿真明确验证极性差异及首次快照语义，不包含光电器件、管脚电气特性、真实中断延迟或现场位流一致性。

现场应在无主回路条件下依次核对：实际使能及寄存器读回、正常/无光接收电平、指定701通道FO与FO_LATCH、主控FB_LATCH、GPIO142引脚及FAULTIsr、核心FAULT及两路PWM撤销、051E/051F/0520位置、故障消失后的完整复位。若在带功率运行中打断点，会暂停控制，不应以此代替连续运行验证。

优先修正701输入极性、GPIO142方向、旧EEPROM读取失败仍覆盖使能、立即故障与后续原因分类的冲突。统一封波如需完全独立于DSP，应另行评估主控fb_block与SHUTDOWN的硬件联动；当前源码没有这条直接联动。
