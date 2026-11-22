# 取消未接入控制位的整包拒绝

日期：2026-11-13。工程：`D:\Code\SST\AnyEyesVersion_DpramCompat`。

## 原因

原控制块先按Word协议有效位提取16字，随后逐字用更小的本机支持掩码检查。第二次检查只要发现某个未接入功能位，就返回`SST_SCREEN_UNSUPPORTED=7`，最终的正常启动赋值尚未执行。

例如word2=1是合法HVAC START，word4=2是文档有效的PQ位。协议对word4的有效掩码是3FFF，但本机功能检查只允许0001，`2 & ~1`非零，于是在执行START前拒绝整包。这会让屏幕携带的其他端口/模式信息连带阻断正常控制。

## 当前处理

`Source/04_App/10_Host/SstDpramCommand.c`已删除第二遍“未接入位非零就返回7”的循环。字段表每行给出协议有效位和本机提取位，一次循环完成：

```c
command_words[index] = words[index] & s_control_masks[index].local_mask;
```

协议范围只用于区分保留内容与未接入的有效功能，并生成诊断位图；不再据此退出本包处理。规则为：

- 本机HVAC启停只取word2/3的bit0，其他端口位不执行、不误路由到HVAC。
- 模式字只兼容提取恒压能力确认位，PQ/下垂/谐波/STATCOM等未接入位忽略，不改变现有`ac_mode`或控制算法。
- 内部开关只取word6/7的bit0/1，外部开关bit2/3忽略，不触发本机维护接口。
- CHB/DAB脉冲、故障复位、合闸许可仍按既有本机位执行。
- PT/CT及电网属性兼容接收描述信息，不动态改变采样比例、PLL或保护。
- 保留字、保留位忽略，原始RAM缓存及共享区不改写，便于对照屏幕。

正常本机请求仍检查远控权限、存储忙、活动故障、完全停机复位条件、开关许可互斥及手动开关联锁。STOP/OFF优先规则保留。DPRAM HVAC START只置`run_enable`，DAB仍由00F8/00F9 bit0独立管理。

此调整针对原16字控制块。原43字遥调的数值范围、负PI编码、整组提交及未实现数值参数拒绝机制保持原样，不把未实现参数伪装成已修改成功。

## 忽略信息

新增RAM成员`g_sst_dpram_screen_result.ignored_control_words`。每笔控制开始时清0，bitN表示wordN存在被忽略的未接入有效位。纯协议保留内容不计入。

补充状态原保留槽37用于发布该值：

|接口|地址/含义|
|---|---|
|Modbus只读|`0x19A5`，16位位图|
|DSP字节地址|`0x6600334A`|
|bit2、bit3|启动/停止字含其他端口位|
|bit4|模式字含未接入功能位|
|bit6、bit7|合/分闸字含外部开关位|
|bit12|端口描述含其他端口位|

例如word2=1、word4=2、其他动作0时：无其他拒绝条件应为1985=0、1988=FFFF、run_enable=1、DAB请求保持原值、19A5=0010。未接入功能没有执行，只是没有连带拒绝本机正常指令。结果0仍只表示本机请求受理，实际PWM读198F。

19A5按原遥测节拍发布，配合epoch读取一致快照。下一笔控制会更新该位图，后续遥调不清此控制诊断。原2400个点地址、块大小及版本保持，19A5只使用已有保留槽。生成工具、CSV、README和DEBUG_STEPS已同步。

## 验证及构建

- `tests/dpram_screen.py`：4配置通过；启用远控配置覆盖每个未接入控制字段与START共存、多个未接入字段混合、仅其他端口不启动HVAC、诊断清零/发布、DAB独立、故障和STOP/OFF优先；负PI及参数事务仍通过。
- `tests/dpram_commands.py`：4种控制源配置通过。
- `tests/dpram_screen_layout.py`：68个原控制实体、ISR/ADC/保护及2400点地址核查通过。
- `tools/ccs_validate.py`：独立临时workspace导入、Debug/Release清理后完整编译通过；各17条既有警告，无新增警告。

固件源输入SHA256：`69aa96b98fe9badd6f25491d56f2de0c162659a84002742cd50b5efbf2cd260d`。编译期间输入一致，结束后核对当前源码与构建相符。

|配置|输出|SHA256|
|---|---|---|
|Debug|`Debug/SST_DpramCompat.out`|`7ecac4418cc6efd34a309cff9f8a8885fc3b260e1aa5507d21270d6f934d3d6e`|
|Release|`Release/SST_DpramCompat.out`|`1cafbd574de3bff84df1624904a0fecffef1bac9a873e6a9a1a6d26367f73775`|

官方EMIFA库、用户TaskSlice/launch改动及10kHz控制路径保留。日志在`build/ccs_validation/`；尚未上板验证。重新下载本次映像后，先无主回路提交上述START与模式位共存的用例，再核对1984计数、1985结果、命令变量及19A5。
