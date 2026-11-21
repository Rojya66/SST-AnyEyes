# 原43字遥调直接赋值说明

工程：`D:\Code\SST\AnyEyesVersion_DpramCompat`。日期：2026-11-17。

## 1. 43和57分别是什么

- **43字**：Word V1.1规定的原遥调块，Modbus地址`0x01FF..0x0229`。屏幕写完后通知左邮箱`0x1FFE=2`。
- **57项**：后来增加的完整参数接口，待应用区`0x1A00..0x1A38`、实际区`0x1A80..0x1AB8`。包含启动、PLL、DAB、保护限值等，需另发APPLY事务。

两者是不同布局，不是收到43字后又收到57字，也不是ARM转发了两次。

此前DSP为了复用扩展接口，先把43字映射到57项候选，再整体校验并经通用参数表赋值。这导致读代码要追多组下标，还可能因不属于原43字的参数配置不合法而拒绝当前遥调。

现在原43字直接赋值，不构造57项候选。57项接口的地址、比例和APPLY/SAVE/LOAD功能保留，供需要完整参数的页面使用。

## 2. 当前调用顺序

```text
屏幕写01FF..0229，最后写1FFE=2
  -> DPRAM_Read()：把43字复制到PORT_Regulate_DATA_Buf.DPRAM_Block2Buf
  -> SstDpramHost_ProcessIncomingBlocks()
  -> SstDpramParameter_DecodeAndApplyLegacyBlock()
       1. 检查远控权限、停机、EEPROM空闲
       2. 检查本43字中实际使用的字段
       3. 在本函数内直接赋值系统方式、保护使能、Udc、频率、PI
  -> SstDpramParameter_EncodeLegacyBlock()：读取实际生效值
  -> DPRAM_Write(20)：回写原43字并通知右邮箱
```

原协议接收和赋值均在`Source/04_App/10_Host/SstDpramParameter.c`前半部分。扩展57项函数排在后半部分；看原屏幕遥调时无需追入扩展函数。

## 3. 原协议的直接赋值关系

原块字号=`Modbus地址-0x01FF`。下表中的words为接收的43字RAM副本。

|Modbus地址|原块字号|最终赋值或硬件调用|
|---|---|---|
|01FF|0|bit0写`Port_RunOrTest`；bit1写`Port_AutoOrManual`|
|0200|1|取F000写`Port_AD_PORT_EN`；变化后调用`Port_ADProtect_EN()`|
|0201|2|取7FFF写`Port_FO_1_EN`|
|0202|3|取7FFF写`Port_FO_2_EN`|
|0203|4|取7FFF写`Port_FO_3_EN`；三组任一变化后调用`Port_FO_EN_Set()`|
|0204|5|写`Port_FB_EN`；变化后调用`Port_FB_EN_Set()`|
|0207|8|`g_chb_dab_command.dc_voltage_ref_v = (float)words[8]`|
|0208|9|`words[9]/10.0F`；开环写输出频率，闭环写PLL额定频率|
|021A|27|`g_chb_dab_params.voltage_kp = (float)(int16_t)words[27]/100000.0F`|
|021B|28|`g_chb_dab_params.voltage_ki = (float)(int16_t)words[28]/100.0F`|
|021C|29|`g_chb_dab_params.current_kp = (float)(int16_t)words[29]/1000.0F`|
|021D|30|`g_chb_dab_params.current_ki = (float)(int16_t)words[30]/1000.0F`|

负PI仍按16位二补码传输。例如`021C=FB1E`解码为-1250，实际`current_kp=-1.25`；`021D=C180`解码为-16000，实际`current_ki=-16.0`。不取绝对值、不额外反号。d/q共用电流PI，Ki仍由控制器执行时乘一次Ts。

只对编码发生变化的浮点参数赋值。这是为了避免屏幕只改Udc时，回读精度把其他PI小数截短。回读四个PI仍使用参数表的既有舍入/二补码编码功能，不经过57项数组，也不执行通用表写入。

## 4. 去掉和保留的检查

已去掉：43转57、旧PI下标减12/减5、Udc/频率中间乘10再除比例、整组校验无关的启动/DAB/PLL/原软件保护参数、为原遥调重复生成完整57项快照。

保留的检查都有具体用途：

- 远控关闭或DPRAM关闭返回5；运行或EEPROM忙返回4，避免运行中半组更新PI及保存时修改参数。
- 改变Udc时要求1..6553V；改变频率时要求1..100Hz，沿用已有接口范围。这是通信允许范围，实际设备整定仍须符合主回路配置。
- 其他端口管理位、未实现的P/Q、谐波、下垂、组2/3 PI等有效参数非零返回7，避免把未实现功能报成成功。
- 保留位和word23..26忽略；AD/FO按有效掩码取值，不因保留位非零拒绝。
- 本块检查在首次实际赋值前完成；错误不留下半笔参数。

PI的全部16位编码均可解释为合法int16，因此没有再加“负PI非法”“零PI非法”等判断。原遥调不修改`run_enable`、`dab_enable`、`fault_reset`、`ac_mode`、保护阈值或10kHz采样周期。

## 5. 屏幕与CCS验证

1. DSP保持Run且控制已停机。读取原43字实际值为底稿，只改目标字段，写回43字，再写`1FFE=2`。
2. 在`SstDpramParameter_DecodeAndApplyLegacyBlock()`入口查看`words[8]`、`words[9]`、`words[27..30]`，这些就是屏幕收到的原值。
3. 在本函数第5步的Udc赋值语句、第6步的PI赋值语句设断点，单步即可看到实际成员改变。不必再追candidate或`WriteEntry()`。
4. 核对`1986`遥调计数递增、`1987=0`。失败时`1988`是原块错误字号；例如8对应0207，9对应0208。
5. 读回0207、021A..021D，核对DSP返回的实际值；只改Udc时，未改PI的原float应完全保持。邮箱20只是回读通知，不能代替1987的执行结果。

断点用于停机参数联调；暂停DSP会同时暂停10kHz控制。正常运行时此路径仅在后台执行，不修改中断入口或控制算法。

## 6. 验证记录

专项用例在`tests/dpram_screen.c`的`test_legacy_parameter_direct_assignment()`，由`python tests/dpram_screen.py`运行，覆盖单相、三相、仿真器权限和DPRAM关闭配置。验证负PI及int16边界、开/闭环频率赋值、未改浮点精度、无关配置不拦截、非法数据不部分应用和运行互锁。

本次构建、测试结果与源输入指纹汇总在`build/parameter_simplification/result.json`；CCS Debug/Release原工程输出为`Debug/SST_DpramCompat.out`和`Release/SST_DpramCompat.out`。此记录是软件验证，未连接ARM/屏幕做现场试验。
