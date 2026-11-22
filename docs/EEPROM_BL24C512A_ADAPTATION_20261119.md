# BL24C512A适配与初始化参数、光纤位置核查

日期：2026-11-19。工程：`D:\Code\SST\AnyEyesVersion_DpramCompat`。

## 1. 硬件及本次结论

现场已确认：BL24C512A接DSP的SDA/SCL，A0/A1/A2均接地，WP接地。

|项目|本版配置|
|---|---|
|器件|BL24C512A，512 Kbit，即65536字节；不是512 KB|
|7位I²C地址|`0x50`|
|线上寻址字节|写`0xA0`，读`0xA1`|
|存储地址|16位字节地址，先发高8位，再发低8位|
|物理写页|128字节；段写自动按边界拆分，防止页内回绕|
|内部写周期|数据手册最大3 ms；异步保存每10 ms启动一次字节写|
|引脚|库编号9=SDA=GPIO0[8]；库编号10=SCL=GPIO0[9]|
|电气方式|SDA/SCL拉低时输出0，置高时改为输入释放，由外部上拉产生高电平|
|WP|接地允许写；不需要X1226的解锁指令或RTC寄存器操作|

参数镜像保留在EEPROM绝对字节地址`0x0180`。**原接口的64字节逻辑页与芯片128字节物理页是两个概念，不能把旧页号乘数改成128。**页6仍是`6×64=0x0180`，不是`0x0300`。独立Identification Page及永久锁定位不参与参数存储。

原底层驱动使用X1226的`AE`存储寻址字节，以及`DE`控制/RTC寻址和解锁流程；这些不能用于地址脚全接地的BL24C512A。本次已换为A0/A1及普通24C512读写，不修改TI官方GPIO驱动库。

## 2. 原程序哪些初始化量会被EEPROM影响

修改前有两条上电加载路径，并非只有现代PI参数的50字镜像。

```text
Initial_Variable
  -> DpramDefaults_InitializeRegulation：建立遥调缓存默认值
  -> FpgaInit_ConfigureModuleLayout：按CHB_PM_*_MASK配置501
  -> ControlDefaults_InitCoefficients
       -> 旧页1/2/3读入遥调缓存
       -> RemoteRegulate_Dispose(1)：再次覆盖位图、模块数、保护使能等

PortStatusInit_DefaultState -> 旧页0恢复端口方式
SystemInit_Runtime -> 现代v1/v2镜像恢复控制参数
```

因此，修改前“PM只由源码宏决定”的说法不完整：宏给出初值，旧页1还有覆盖机会。旧原始页没有CRC，且原调用未检查I²C读取返回值，读取失败可能把未初始化的局部缓存当作参数使用；空白的新芯片也不能直接按旧格式解包。

以下地址均为**EEPROM字节地址**，不是屏幕Modbus地址。旧页内16位值按高字节在前打包，与统一镜像的小端格式不同。

|旧区域|内容|修改前的作用|
|---|---|---|
|页0，`0008..000B`|主工作方式、STATCOM方式、APF方式及一个已不用的许可字|恢复`Port_Main_WorkMode`等；许可字现已忽略|
|页1，`0040..004B`|COM_SYS_MODE、AD_PROT_EN、三组FO_EN、FB_EN|方式及保护使能通过旧遥调缓存写回活动对象/FPGA|
|页1，`004C..0065`|P/Q、V/F给定、谐波及STATCOM原给定|旧控制对象/缓存；其中部分已不属于现有CHB/DAB控制功能|
|页1，`0066..0075`|V0/F0/KF/KV和Reserved_1[0..3]|在HVAC旧分支中还承担六组501位图、端口存在位图、电流限幅等用途，见下表|
|页2，`0080..0097`|三组电压/电流PI，共12字|旧PI兼容字段，不等于当前`g_chb_dab_params`的直接PI值|
|页3，`00C0..00C7`|`EER2_DATA[4]`|旧扩展缓存；当前屏幕未把这些字映射为有效控制对象|
|统一v1/v2区，`0180`起|42字控制参数/给定；v2另加8字保护/巡检参数|最后恢复当前CHB/DAB控制参数；原来不包含PM、AD、FO、FB|

旧页1中与接线最相关的隐藏复用关系：

|EEPROM旧地址|遥调缓存字段|`RemoteRegulate_Dispose`后的用途|
|---|---|---|
|`006E..006F`|`Reserved_1[0]`|`HA_PM_A1`|
|`0070..0071`|`Reserved_1[1]`|`HA_PM_A2`|
|`0066..0067`|`PORT_V0_SET`|`HA_PM_B1`|
|`0068..0069`|`PORT_F0_SET`|先除10，再作为`HA_PM_B2`|
|`006A..006B`|`PORT_KF_SET`|先除10，再作为`HA_PM_C1`|
|`006C..006D`|`PORT_KV_SET`|先除10，再作为`HA_PM_C2`|
|`0072..0073`|`Reserved_1[2]`|`Port_OnState`，原四端口存在/联动位图|
|`0074..0075`|`Reserved_1[3]`|旧`Idst_Max`限流，旧分支限制在10..95；不是当前电压环限流参数|

本版不再自动加载这些无CRC的原始页。`X1226_RW.C`保留显式旧格式兼容函数，并检查读成功及非全FF；正常启动、屏幕SAVE/LOAD均不调用它们。**不自动猜测、迁移旧原始页里的接线。**已知v1/v2统一镜像仍可读取。

## 3. 修改后的唯一上电恢复流程

```text
SystemInit_Hardware
  -> GPIOBankPinInit：SDA/SCL上电即释放
  -> Eeprom_Initialize：释放总线，启动CPU周期计数延时时基

SystemInit_Runtime
  -> Initial_Variable / Variable_RST：建立完整源码默认及封波状态
  -> ChbDabControl_Init：清启停请求，初始化控制状态
  -> SstParameters_LoadPersistent（仅CHB_LOAD_EEPROM_PARAMETERS=1）
       -> SstParameterStore_LoadPersistent / StoreLoad
            读头、识别版本/固定长度、读载荷、CRC校验
            -> 校验控制参数、保护参数、v3硬件参数
            -> 全部通过后才一起应用
  -> FaultLatch_Clear(0,1)：最终接线和使能确定后启动原两阶段锁存复位
  -> 初始化屏幕实际值/候选区
  -> 开放TAD、FAULT和可选DPRAM中断
```

空白、器件无应答、CRC不符、版本/长度错误或任一参数越界：加载失败，不部分应用，保留本次启动已经建立的源码默认。失败结果仍如实记录，不伪报加载成功。手动LOAD失败时保留手动LOAD前的活动值。

`CHB_LOAD_EEPROM_PARAMETERS=0`只关闭上电恢复，便于重新按源码配置接线；显式屏幕保存/加载服务仍存在。不会每次开机擦除或重写EEPROM。

上电复位移到最终参数加载之后，避免默认FO使能先对未接通道形成锁存、随后加载新FO使能却遗留旧锁存。运行中的手动LOAD不隐含清故障；接线/保护使能改好后仍按正常流程复位故障。

## 4. 光纤位置、模块数量和保护位图

### 4.1 六组501调制/采样通道

|组|DSP变量|端口控制器501插件编号|源码默认|当前模块数|
|---|---|---|---|---|
|A1|`HA_PM_A1`|5|`0007`|3|
|A2|`HA_PM_A2`|6|`0007`|3|
|B1|`HA_PM_B1`|9|`0000`|0|
|B2|`HA_PM_B2`|10|`0000`|0|
|C1|`HA_PM_C1`|13|`0000`|0|
|C2|`HA_PM_C2`|14|`0000`|0|

默认定义在`Source/04_App/04_Control/ChbDabControl.h`的`CHB_PM_*_MASK`，不是`ChbBuildConfig.h`。每张501只有低8位有效：bit0..7分别对应该卡通道0..7，按面板从1编号时为第1..8口。例如`0x0085`表示通道0、2、7有效，不能因为只有3个模块就改写为`0x0007`。

两组是同相的两张501卡，不是某个UNIT的左右桥臂。每相模块总数为同相两张位图的置位数之和。例如当前A相是3+3=6个模块。

`FpgaInit_ApplyModuleLayout()`负责全部相关更新：

1. 写六个`HA_PM_*`。
2. 写六张501的`FPGA_Interface[插件号-2].PM`。
3. 按低8位置位数重算`NHA_module_A/B/C`。
4. 同相两张501的`NUM`均写入该相总数。
5. 重算`HA_ReferenceDEF_A/B/C = ReferenceDEF × NHA_module_*`。
6. 同步旧遥调缓存中原来复用的接线字段，避免默认提交时用过期缓存覆盖。

EEPROM保存位图，不单独保存NUM、相模块数或参考基值；加载时重新推导，避免存储互相矛盾的数据。

### 4.2 701的故障光纤和主控FB是另外两层配置

|活动变量|源码默认|真实写入|
|---|---|---|
|`Port_AD_PORT_EN`|`7000`|`Port_ADProtect_EN()` -> 202的`AD_PROT_EN`|
|`Port_FO_1_EN`|`3F7F`|`Port_FO_EN_Set(3,...)` -> A相701（插件8）`FO_EN`|
|`Port_FO_2_EN`|`3F7F`|同上 -> B相701（插件12）|
|`Port_FO_3_EN`|`3F7F`|同上 -> C相701（插件16）|
|`Port_FB_EN`|`4441`|`Port_FB_EN_Set()` -> 主控`FB_EN`|

FO bitN对应701的`OP_IN[N]`，面板从1编号时为第N+1口。`3F7F`启用bit0..6和8..13，共13路；不是当前6个A相模块的自动适配值。主控`4441`启用bit0、6、10、14，即采样板及三块701，共四路。

本版FO参数校验沿用现有屏幕协议的低15位有效约定，bit15保留；FB的16位均有效。701硬件本身有16路输入，不应把软件点表约定误认为硬件只有15路。

**PM、FO_EN、FB_EN不会互相自动生成，也不会随单相宏自动裁剪。**模块调制/采样光纤接入501的位置、模块故障光纤接入701的位置需要分别登记。

旧原因分类按701低8位选择同相第一张501，高8位选择第二张501。因此除屏幕位置显示外，原详细故障分类还要求这种分组接线一致。例如A1接501通道0..2、A2也接各自501通道0..2，且对应故障光纤分别接A701输入0..2、8..10时，A相FO位图应为`0707`；只有实物接线确实如此才使用这个示例。

屏幕`051E/051F/0520`仍分别显示三块701锁存与当前FO使能相与的物理输入位图。本次存储适配不改变其含义，也不改变701输入极性、GPIO142方向或原详细原因分类流程；相关未完成硬件/保护问题仍按[保护核查记录](UNIT_701_PROTECTION_AUDIT_20261119.md)单独处理。

### 4.3 哪些配置不依赖EEPROM

- 插件硬件编号、`FPGA_Interface`地址和插件ID初始化来自源码。
- `SST_PHASE_A_ONLY_TEST`、通信协议选择、DPRAM使能、开关输出/反馈宏为编译配置。
- 10kHz TAD/FPGA周期、GPIO复用、QF1/KT1的IO通道及极性不由本镜像修改。
- PI积分历史、PLL/SOGI内部状态、采样滤波历史、故障记录不是持久化配置。
- UNIT硬件内部保护阈值不因主控EEPROM保存而改变。

## 5. 新v3镜像的具体地址

EEPROM起点`0180`；所有地址为绝对**字节**地址；16位载荷低字节在前。总长140字节，范围`0180..020B`。

|EEPROM区域|内容|
|---|---|
|`0180..0181`|魔数`53 54`|
|`0182`|版本`03`|
|`0183`|长度`8C`，十进制140|
|`0184..0185`|保留0|
|`0186..01D9`|原42字：37个参数及5个非启停给定|
|`01DA..01E9`|原8字：两级软件OC门限/时间、OV门限/时间、预充巡检低压/高压|
|`01EA..0209`|新增16字硬件参数，见下表|
|`020A..020B`|CRC16，初值FFFF，多项式A001；覆盖前138字节|

16个硬件字的固定顺序：

|镜像载荷索引|EEPROM低字节地址|内容|恢复位置|
|---|---|---|---|
|50|`01EA`|系统管理方式bit0运行/测试、bit1自动/手动|`PortRunFLAG`两个管理位|
|51|`01EC`|AD保护使能|`Port_AD_PORT_EN`及202寄存器|
|52|`01EE`|A相FO使能|`Port_FO_1_EN`及A701|
|53|`01F0`|B相FO使能|`Port_FO_2_EN`及B701|
|54|`01F2`|C相FO使能|`Port_FO_3_EN`及C701|
|55|`01F4`|FB使能|`Port_FB_EN`及主控|
|56|`01F6`|A1通道位图|`HA_PM_A1`及501插件5|
|57|`01F8`|A2通道位图|`HA_PM_A2`及501插件6|
|58|`01FA`|B1通道位图|`HA_PM_B1`及501插件9|
|59|`01FC`|B2通道位图|`HA_PM_B2`及501插件10|
|60|`01FE`|C1通道位图|`HA_PM_C1`及501插件13|
|61|`0200`|C2通道位图|`HA_PM_C2`及501插件14|
|62|`0202`|端口存在/联动位图|`Port_OnState`，低4位|
|63|`0204`|原主工作方式|`Port_Main_WorkMode`，0..3|
|64|`0206`|原STATCOM子方式|`Port_STATCOM_WorkMode`，0..3|
|65|`0208`|原APF子方式|`Port_APF_WorkMode`，0..2|

此表的EEPROM载荷索引与屏幕57项参数索引不是同一张表。屏幕57项中的第50项是临时快速保护开关，不保存；第51..56项对应管理方式及AD/FO/FB，已加入v3保存。PM没有借用屏幕原43字中的保留字作为新遥调点，本次不移动、不扩展屏幕参数地址。

v1的92字节镜像只恢复42字；v2的108字节镜像另恢复8字原保护/巡检。二者均不改变本次启动的硬件默认。新保存统一写v3。

不保存`run_enable`、`dab_enable`、`fault_reset`、物理QF1/KT1输出及`g_chb_dab_protection_enable`临时旁路。管理位“运行方式”不等于启动命令；上电仍保持停机。

## 6. 屏幕交互及现场验证

### 6.1 正常保存、加载

1. 完全停机：CHB/DAB请求均为0、两路PWM均关闭，状态STOPPED或FAULT；`1B04=0`。
2. 先把屏幕参数应用生效并核对实际值。原43字遥调仍按完整块提交至`1FFE=2`；扩展候选区仍用`1B01=1`应用。
3. 仅写`1B01=2`。DSP受理后把操作字清0，`1B04=1`表示保存仍在进行；无需填写`1B00`序号。
4. 等待`1B01=0`且`1B04=0`，再看`1B03=0`、`1B06=2`才确认保存成功。操作字清0本身不是成功。
5. v3共141次字节写操作（先使魔数无效、写其余139字节、最后提交魔数）及140次读回，共281个10ms节拍，名义约2.8秒；主循环繁忙时更长。每节拍最多推进一步，不在服务内等3ms写周期。
6. 停机修改一个PI或预充阈值并应用，然后只写`1B01=3`加载，确认恢复保存值。加载完成后用`1B01=4`刷新候选区再继续编辑。
7. 在`CHB_LOAD_EEPROM_PARAMETERS=1`下断电重启，检查参数和六组PM/NUM、三组FO、FB、AD与保存值一致，启停请求仍为0。

加载使用带返回状态的连续读；保存读回使用带返回状态的单字节读。无ACK和读到FF是两个情况，均不会靠“缓存碰巧相同”伪报成功。

未保存的新芯片上电加载结果6是正常现象：没有有效镜像，源码默认保留。首次完成上述保存及断电验证后，再作为正常持久化使用。

### 6.2 换光纤接线或改变模块数量

有效v3镜像优先于源码默认。已保存后仅修改`CHB_PM_*`宏再下载，可能又被旧镜像覆盖。需要重新建立接线时：

1. 无主回路、完全停机，在源码设置六组`CHB_PM_*_MASK`，并在`DpramDefaults.c`设置对应FO/FB默认。
2. 暂时令`CHB_LOAD_EEPROM_PARAMETERS=0`编译下载，让这次上电采用新源码接线。
3. 核对六组`HA_PM_*`、三个`NHA_module_*`及六张501的`PM/NUM`；核对三块701的`FO_EN`和主控`FB_EN`。单独核对UNIT故障返回光纤与701位置。
4. 配好实际PI/给定/保护参数后，从屏幕写`1B01=2`并确认结果0，建立新v3镜像。
5. 恢复`CHB_LOAD_EEPROM_PARAMETERS=1`编译下载，断电重启复核。

PM当前没有屏幕编辑点；上述方法明确按源码重建接线。调试器仅改`HA_PM_*`不会立即同步实际FPGA的PM/NUM；本版的初始化及LOAD才通过统一应用函数同步，不能把改RAM当作已修改硬件。

### 6.3 CCS观察对象

|观察量|解释|
|---|---|
|`g_sst_eeprom_status.error`|0底层正常；1总线被拉低；2无ACK；3地址/长度错误；4同步段写的写忙轮询超时|
|`.address`|最近一次访问的EEPROM绝对字节地址；完整保存校验末尾为`020B`|
|`.stage`|0最近事务完成；1器件地址；2地址高字节；3低字节；4写数据；5重复START后的读方向寻址|
|`.device_byte`|实际使用的寻址字节；写A0、读A1|
|`storagePosition`|`SstParameterStore.c`静态步序，完成时281；不是旧X1226内部等待位置|
|`storagePending` / `storageResult`|正在保存及结果；也可从屏幕`1B04/1B03`查看|
|`HA_PM_*`、`NHA_module_*`|六组实际通道位图和三相总数|
|`Port_FO_1/2/3_EN`、`Port_FB_EN`、`Port_AD_PORT_EN`|最终活动保护使能，再与FPGA寄存器核对|

若结果6，先看底层error和stage；WP已接地后，仍需确认DSP这条总线的供电、上拉及SDA/SCL连接。不要把底层无应答改成“成功”来绕过。最后一次I²C成功但结果仍6也可能是镜像CRC、版本或读回数据不符。

## 7. 修改文件、编译与验证边界

|文件|修改职责|
|---|---|
|`02_Peripheral/02_Storage/EepromConfig.h`|地址脚、GPIO、容量、128字节物理页及64字节逻辑页|
|`02_Peripheral/02_Storage/EEProm.C/.H`|BL24C512A开漏软件I²C、应答检查、跨页写、错误观察对象；取消原RTC/解锁寻址|
|`02_Peripheral/02_Storage/X1226_RW.C`|旧格式显式读取检查成功与空白；退出自动启动路径|
|`03_System/00_Board/SstBoard.c`|软件I²C两脚初始为输入释放|
|`03_System/01_OS/SystemInit.c`|I²C初始化及最终配置后的启动锁存复位顺序|
|`04_Control/ControlDefaults.c`、`PortStatusInit.c`|取消无校验旧页自动加载/首次写|
|`00_FPGA/FpgaInit.c/.h`|抽出统一位图应用及模块数/参考基值重算|
|`01_FuncCode/SstHardwareParameters.c/.h`|16个硬件参数的明确读取、校验及RAM/FPGA赋值|
|`01_FuncCode/SstParameterStore.c`|v3的66字/140字节镜像，v1/v2兼容，完整校验、分步保存及读回|

表内Source路径从工程`Source`起算。`SstControlConfig.h`同步删除失效旧EEPROM宏，DPRAM旧回读处删除无内容条件编译；屏幕控制和调参地址、比例不变。

验证命令：

```powershell
python -X utf8 tests/eeprom_bl24c512a.py
python -X utf8 tests/eeprom_module_layout.py
python -X utf8 tests/parameter_store_migration.py
python -X utf8 tests/dpram_screen.py
python -X utf8 tests/dpram_maintenance.py
python -X utf8 tests/protocol_regression.py
python -X utf8 tests/parameter_precision.py
python -X utf8 tests/firmware_trace.py
python tools/build.py --configuration Debug --variant bl24c512a
python tools/build.py --configuration Release --variant bl24c512a
```

驱动测试使用实际驱动和边沿级I²C从机模型，验证A0/A1、16位地址、128字节写页分段、末字节NACK、开漏、无ACK退出及有限写忙轮询。布局测试执行实际`FpgaInit_ApplyModuleLayout`和硬件参数模块，遍历256组稀疏位图，核对PM/NUM、参考基值及兼容缓存。镜像测试覆盖空白/NACK/CRC/非法字段不部分应用、v1/v2兼容、v3接线及保护恢复、忙互锁和每拍一步预算。屏幕测试覆盖单相/三相、权限、重复保存及保存失败后重试。

本次不向10kHz `FPGAIsr`或`FAULTIsr`加入任何EEPROM操作，不关闭全局中断。位操作等待允许中断抢占；不保证后台负载增加时保存仍恰好2.81秒。主机测试不验证真实DSP中断执行时间、I²C电气波形、实际芯片写入或断电保持，这些需要按第6节上板验收。

Debug与Release输出分别为`build/Debug/bl24c512a/SST_DpramCompat.out`、`build/Release/bl24c512a/SST_DpramCompat.out`；编译日志和输入指纹在各自同目录。具体编译与回归结果见[验证记录](EEPROM_BL24C512A_VALIDATION_20261119.md)。
