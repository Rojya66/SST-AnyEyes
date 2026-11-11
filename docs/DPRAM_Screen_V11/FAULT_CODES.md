# SST故障码与Word故障位

详细码沿用当前SstFault报告模块；本次重打包旧PORT_FAULT对外语义，内部FaultCheck及控制封波不改。

|码值|宏|含义|
|---|---|---|
|19101|`SST_FAULT_HW_UNCLASSIFIED`|未能细分的硬件故障五位上报码。|
|19102|`SST_FAULT_HW_PORT_OC`|端口硬件过流五位上报码。|
|19103|`SST_FAULT_HW_PORT_OV`|端口硬件过压五位上报码。|
|19104|`SST_FAULT_HW_MODULE`|模块汇总故障五位上报码。|
|19105|`SST_FAULT_HW_MODULE_OC`|模块过流五位上报码。|
|19106|`SST_FAULT_HW_MODULE_OV`|模块过压五位上报码。|
|19107|`SST_FAULT_HW_MODULE_OT`|模块过温五位上报码。|
|19108|`SST_FAULT_HW_CHB_BRIDGE`|模块工频 CHB 桥故障五位上报码。|
|19109|`SST_FAULT_HW_DAB_BRIDGE`|模块 DAB 桥故障五位上报码。|
|19110|`SST_FAULT_HW_MODULE_SINGLE`|单模块故障五位上报码。|
|19111|`SST_FAULT_HW_MODULE_UV`|模块欠压五位上报码。|
|19112|`SST_FAULT_HW_MODULE_OV2`|模块二级过压五位上报码。|
|19113|`SST_FAULT_HW_FAN`|风扇故障五位上报码。|
|19114|`SST_FAULT_HW_CONTROLLER`|控制器故障五位上报码。|
|19115|`SST_FAULT_HW_AUX_SUPPLY`|辅助电源故障五位上报码。|
|19116|`SST_FAULT_HW_EPO`|紧急关断输入故障五位上报码。|
|20103|`SST_FAULT_PRECHARGE_TIMEOUT`|预充超时五位上报码，沿用参考 20103。|
|29102|`SST_FAULT_PLL_TIMEOUT`|PLL 锁定超时五位上报码。|
|29103|`SST_FAULT_SW_AC_OC`|软件交流过流五位上报码。|
|29104|`SST_FAULT_SW_DC_OV`|软件直流过压五位上报码。|
|29105|`SST_FAULT_BAD_CONFIGURATION`|控制配置非法五位上报码。|
|29106|`SST_FAULT_PHASE_SEQUENCE`|相序异常五位上报码。|
|29107|`SST_FAULT_FREQUENCY`|频率异常五位上报码。|
|29108|`SST_FAULT_SWITCH_FEEDBACK`|进线/旁路反馈不符，五位上报码；不占用已有相序及频率故障码。|
|39101|`SST_FAULT_CONTACTOR`|接触器故障五位上报码。|
|39102|`SST_FAULT_BREAKER`|断路器故障五位上报码。|
|39103|`SST_FAULT_COMMUNICATION`|通信故障五位上报码。|
|39104|`SST_FAULT_SYNC`|同步故障五位上报码。|

等级编码：0=一级，1=二级，2=三级，3=四级，4=无故障。当前码表最多32项；191xx硬件来源、291xx软件/控制原因、391xx接触器/通信类来源，20103预充超时。

|外部Word位|来源/说明|
|---|---|
|0接触器|内部Port bit0或FaultType bit8|
|1断路器|内部Port bit1或FaultType bit9|
|2风机、3控制器、4供电、5EPO|Port对应位；控制器同时取FaultType bit10|
|6模块|Port bit6，或FaultType模块汇总/单模块/OC/OV/OT/UV/二级OV类|
|7逆变器|Port bit7，或FaultType CHB/DAB桥故障|
|8相序、9频率|Port对应位|
|10端口过流|Port bit10或内部复用bit14、FaultType bit1、HVAC_OC_Fault、核心快速OC|
|11欠压|Port bit11或FaultType bit14|
|12过压|Port bit12或内部复用bit13、FaultType bit0、HVAC_OV_Fault、核心快速OV|
|13通信|FaultType bit12；不再复制内部Port bit13|
|14同步|FaultType bit11；不再复制内部Port bit14|
|15总故障|任一上述故障、内部原字/分类非零，或核心任一故障/FAULT态|

核心原因0无、1预充超时、2PLL超时/失锁、3快速OC、4快速OV、5硬件汇总、6非法配置、7开关反馈。预充、PLL、配置、反馈等不伪装成其他旧位；用总故障+19C3原因+19C8起详细码解释。

原始诊断地址：19C1 Port_RealFault、19C2 FaultTypeFLAG、19E8软件OC、19E9软件OV。当前首码19C4不是永久首触发锁存；当前表在19C8..19E7，原历史扩展起点0x1780，仍用旧并口快照格式。
