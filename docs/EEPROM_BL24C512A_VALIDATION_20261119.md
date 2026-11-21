# BL24C512A适配验证记录

日期：2026-11-19。实现说明见[EEPROM适配与光纤配置](EEPROM_BL24C512A_ADAPTATION_20261119.md)。

## 编译

使用TI C6000 8.3.12，三种构建均编译96个源文件，编译/链接错误0，构建期间输入未变化。

|配置|附加宏|结果|输出|
|---|---|---|---|
|Debug bl24c512a|无，当前默认单相|通过|`build/Debug/bl24c512a/SST_DpramCompat.out`|
|Release bl24c512a|无，当前默认单相|通过|`build/Release/bl24c512a/SST_DpramCompat.out`|
|Release bl24c512a_three_phase|`SST_PHASE_A_ONLY_TEST=0`|通过|`build/Release/bl24c512a_three_phase/SST_DpramCompat.out`|

每种构建17条警告，来自既有TI库头的续行空白及原DPRAM/采样/故障代码中的未使用变量；本次EEPROM和新硬件参数模块没有编译警告。

三个固件对应同一个源码/工程输入指纹：

`4bcb26fc2988e816535657ef640593c7f1e537c0a050328741277606227e204f`

## 主机测试

- `tests/eeprom_bl24c512a.py`：通过，日志`build/bl24c512a_validation/eeprom_bl24c512a.log`。
- `tests/eeprom_module_layout.py`：通过，日志`build/bl24c512a_validation/eeprom_module_layout.log`。
- `tests/parameter_store_migration.py`：通过，日志`build/bl24c512a_validation/parameter_store_migration.log`。
- `tests/dpram_screen.py`：通过，日志`build/bl24c512a_validation/dpram_screen.log`。
- `tests/dpram_maintenance.py`：通过，日志`build/bl24c512a_validation/dpram_maintenance.log`。
- `tests/protocol_regression.py`：通过，日志`build/bl24c512a_validation/protocol_regression.log`。
- `tests/parameter_precision.py`：通过，日志`build/bl24c512a_validation/parameter_precision.log`。
- `tests/firmware_trace.py`：通过，日志`build/bl24c512a_validation/firmware_trace.log`。

其中布局测试执行实际模块位图函数并覆盖256组稀疏PM；镜像测试覆盖v1/v2/v3、空白/NACK/CRC/非法字段、写入ACK但数据不变、读回无ACK、忙互锁、运行中断保存及不部分应用。屏幕4种宏组合、并口维护4种组合、串口5种协议/权限组合均通过。固件调用轨迹测试覆盖4种构建、每种17个启动/调度/ISR场景，确认上电恢复先于锁存复位、复位先于TAD开放。

本次EEPROM/硬件参数/布局接口的函数、参数、变量注释范围检查为0项遗漏。全工程注释扫描另有28项不在本次EEPROM范围内的记录，保留在验证JSON中；不将其表述为全工程注释审计通过。

静态复核：ControlDefaults/PortStatusInit已无旧页自动恢复调用；控制核心和ISR源文件没有EEPROM读写或参数镜像服务调用。文档16个新增硬件参数的EEPROM字节地址逐项匹配载荷公式，相关文档链接有效。

## 结果与边界

机器记录：`build/bl24c512a_validation/result.json`，包含构建输入指纹、三个.out文件SHA256、测试结果及范围外注释记录。

尚未烧录实板，未验证实际芯片读写、电气波形和掉电保持；未测量新固件的DSP中断执行时间或栈水印。上板验收按适配说明第6节执行。
