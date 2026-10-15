# CHB左右桥臂、UNIT输出接口及完整信号路径核对

核对日期：2026-10-15。DSP基准为`cfd5caf306b81f3aa95911be7869f90110ee6f5e`；UNIT源码为`D:/Code/FPGA/Pango/UnitBoard_DAB_Test_NoInterlock`。本报告分析现有程序，没有修改DSP或FPGA生产源码，也没有读取现场FPGA配置或测量实板波形。

## 1. 物理左右的共同定义及确定范围

- **物理左桥臂**：桥臂中点接输入高压侧。
- **物理右桥臂**：桥臂中点接下一个子模块的物理左桥臂；末级按实际系统接线处理。
- 本文另称`PWML/SVG_PL`为**逻辑L**，`PWMR/SVG_PR`为**逻辑R**，避免把代码名称当成外部接线事实。
- `A1/A2`是同一相的两块501光通信插件/两组模块，不是CHB的左右桥臂。每个UNIT内部都生成两组CHB桥臂信号。

当前UNIT代码确实执行了一次左右交换：`main.v:601`注释为“模块倒接，SVG左右桥臂互换”，602～624行的实际端口连接与注释一致。因此**逻辑L最终输出到H2/J3，逻辑R最终输出到H1/J2**，上下管顺序未交换。

控制板原理图不包含外部功率主回路、驱动排线及功率器件门极接线，故还不能从这些软件文件确认“输入高压侧”究竟接了J2还是J3。若输入高压侧接J3，则物理左=逻辑L、物理右=逻辑R；若输入高压侧接J2，则物理左=逻辑R、物理右=逻辑L。不能只凭`L/R`、H1/H2编号或原理图上的绘图位置判断。

## 2. 已确定的四路PWM接口表

原理图：`D:/Code/Gemini/SST/NewPlatform/Schematic/UNIT.pdf`第4页FPGA BANK15、第11页Power scheme。管脚约束：UNIT `source/main.fdc:288-308`；对应布局管脚文件也一致。

| 501/UNIT逻辑来源 | UNIT死区后信号 | UNIT顶层输出 | FPGA球脚 | 原理图网络 | 本板输出路径 | 排针PWM脚 |
|---|---|---|---|---|---|---|
| 逻辑L上管 | `SVG_PLU` | `F_H2T1` | L19 | `PWM+_T2` | U26 A3→B3，R211=0Ω | **J3-11** |
| 逻辑L下管 | `SVG_PLD` | `F_H2T2` | L20 | `PWM-_T2` | U26 A4→B4，R212=0Ω | **J3-15** |
| 逻辑R上管 | `SVG_PRU` | `F_H1T1` | K18 | `PWM+_T1` | U26 A1→B1，R209=0Ω | **J2-11** |
| 逻辑R下管 | `SVG_PRD` | `F_H1T2` | K19 | `PWM-_T1` | U26 A2→B2，R210=0Ω | **J2-15** |

U26为SN74LVC8T245，A侧3.3V、B侧5V，DIR固定高，OE通过R93=0Ω接地，为A→B非反相输出。本板PWM路径没有额外逻辑取反；外接驱动板的输入有效电平仍须核对其电路。`PWM+/-_T1/T2`是两路上下管命名，不是差分PWM对。

J2/J3的5、7脚为15V_DRIVER，9、13脚为SO1/SO2故障返回，11、15脚才是PWM；偶数脚2～20为GND，1、3、17、19为NC。不要把9/13脚当PWM。该20针接口不直接连接功率器件门极，而是连接外部驱动电路。

## 3. DSP到光通信插件

`Source/03_System/02_ISR/Redef_Interrupt.c:251-270`的10kHz `FPGAIsr()`调用`ChbDabControl_FastStep()`。控制核心在运行态选择开环或闭环，最终都进入`Chb_WriteNormalizedReferences()`，没有由DSP直接输出CHB四路门极PWM。

- 开环：`ChbDabControl.c:1261-1285`计算`qA=1+M*sin(theta)`，B/C相各偏移120°。
- 闭环：同文件约1480～1510行把电压环/电流环及电网前馈得到的相电压换成`q=1+vphase/(N*Udc_ref)`。
- 统一写入：394～425行把q限幅到0.02～1.98，并写`Reference=(uint16_t)(q*HA_ReferenceDEF_phase)`；同相两块插件收到同一个相参考。
- `FpgaInit.c:79-174`按模块位图置位数求N，再令`HA_ReferenceDEF_phase=N*1250`。`SstControlConfig.h:17/35`为`ControlTAD=5000`、`ReferenceDEF=1250`。

当前`ChbDabControl.h:10-20`为A1=0x0001、A2=0，A相N=1；B1/C1=0x007F、B2/C2=0x003F，B/C各N=13。当前单UNIT连接应核对A1插件的光纤通道0，不应把A2理解为第二个桥臂。

EMIFA寄存器以`FPGA_Interface[]`访问，基址0x60000000，每插件窗口0x200字节。A1是`FPGA_Interface[5-2]`，基址0x60000600：

| 字段 | 插件内字偏移 | A1 DSP字节地址 | 作用 |
|---|---|---|---|
| NUM | 0x02 | 0x60000604 | 全相模块数 |
| PM | 0x03 | 0x60000606 | 本插件有效光纤通道位图 |
| Reference | 0x04 | 0x60000608 | 相调制参考，非直接门极状态 |
| DC_Log | 0x06 | 0x6000060C | bit0排序方向、bit8电流方向 |
| TAD | 0x07 | 0x6000060E | 原算法计数5000 |
| PM_CTL | 0x0D | 0x6000061A | CHB/DAB使能、分频、复位、模式 |

布局依据为`FpgaRegister.h:153`起的结构体、`FpgaRegister.c:5-7`及链接map。MainControl的`source/main403.v:204-220,1517`按窗口译码并把片选送往对应插件；这段不交换CHB左右信号。

## 4. 501分解左右参考并经同一根光纤发出

中间环节核对了`D:/Code/FPGA/Pango/501_openlooptest`和`501_compatible_build`，尚未确认现场下载版本。二者的`source/pwm.v`逐字节一致，SHA256均为`f2a52869d4d1f0721c2fdac73138211ddcc6e799503f558d49a6a36ccfe7baec`；下述CHB参考分解和标签规则相同。main501的DAB模式、发送掩码等不能据此认定完全相同。

501 `main501.v:1280-1291`按寄存器地址接收DSP数据；1515～1529行计算：

```text
Z    = NUM * TAD / 2
LREF = Reference
RREF = Z - Reference
```

`main501.v:1600-1611`把LREF/RREF、模块总数、本卡数量、电流方向和排序方向交给`pwm.v`，算法进行电平分配、交点计算及模块排序。**LREF/RREF不是两个原样透传的占空比，需经过该算法。**

`pwm.v:619-705`按分配结果生成16位PWML/PWMR：

| 位 | 逻辑L帧 | 逻辑R帧 |
|---|---|---|
| 15:14 | `01` | `10` |
| 13 | 周期开始的初始逻辑电平FPL | 初始逻辑电平FPR |
| 12:0 | 翻转计数TL | 翻转计数TR |

501 `main501.v:595-619`在同一个UNIT光纤通道中，约T+25μs发送逻辑L，T+31μs发送逻辑R；其余时隙发送控制字和DAB参数。它们是同一条串行链路的不同标签，不是两根“CHB左/右PWM光纤”。实际发送使能取决于501版本及PM/TX_EN配置。

## 5. UNIT解码、生成互补信号并执行交换

UNIT `source/main.v:154-178`接收16位光纤数据：高两位01的字进入`reg_PWM_SVG_L`，高两位10进入`reg_PWM_SVG_R`，保留低14位。249～280行在计数回卷或外部同步时锁存初始电平与翻转计数。

普通模式`WORK_MODE=0`时，周期起点取FPL/FPR，到TL/TR翻转一次；`WORK_MODE=1`时还在`SVG_TAD-TL/TR`翻转回去。该位对应DSP `Module_PM_CTL.ONE`的bit12，不是DSP的`ac_mode`。当前DSP初始化PM_CTL=0x0F00，正常路径没有调用`Module_TestMode()`，故为普通单交点模式。`SVG_DIV=0`时UNIT周期计数为5000、时钟100MHz，载波约20kHz，与DSP 10kHz控制任务频率应区分。

随后路径为：

```text
PWML → reg_PWM_SVG_L → FPL/TL → SVG_PL
     → deadzone(PL, ~PL) → SVG_PLU/PLD
     → antishake → PWM_H2T1/T2 → F_H2T1/T2 → L19/L20 → J3-11/15

PWMR → reg_PWM_SVG_R → FPR/TR → SVG_PR
     → deadzone(PR, ~PR) → SVG_PRU/PRD
     → antishake → PWM_H1T1/T2 → F_H1T1/T2 → K18/K19 → J2-11/15
```

死区参数为120个100MHz周期，名义1.2μs；最小脉冲滤波150周期，名义1.5μs。同步及滤波还会引入延时，不能把名义参数当作示波器测得的完整沿间隔。`deadzone`实例本身叫H1/H2，但真正端口归属应以后级antishake和F_HxTy连线为准，实例名不会覆盖后级的交换。

## 6. 当前单UNIT配置的额外问题

本节是在上述两份501源码条件下的算法结论，不等于已经确认现场位流行为。

当前新DSP控制入口只写Reference；旧`Write_ControlParameter_50Hz()`中虽有DC_Log方向/排序写入，全Source没有活动调用。501 `main501.v:116`的`reg_DC_LOG`默认0，因而在FPGA新上电、且未被调试器/原始寄存器工具/先前运行改写时为ORI=0、ORDER=0。应实际读取A1的0x6000060C确认，不依赖猜测。

501 `pwm.v:447/531`的`N-2-OnNum`在N=1时发生无符号下溢；ORDER=1分支577行又取索引1，而单模块有效层只有0。这份多模块算法没有N=1专用处理。为核对这一点，使用Icarus Verilog直接仿真原`pwm.v`，只用组合商/余数模型替代除法IP，运行ORI×ORDER×三种参考共12组合；夹具和日志在`build/chb_path_audit/num1_trace_tb.v`、`num1_trace.log`。仿真用于算法稳态核验，不证明IP延迟、板上时序或实际烧录文件。

默认ORI=0、ORDER=0，A相N=1的结果为：

| 归一化q | 有符号调制m=q−1 | LREF/RREF | PWML | PWMR | 已使能时UNIT结果（忽略死区/滤波） |
|---:|---:|---|---|---|---|
| 0.8 | −0.2 | 1000/1500 | 0x63E8 | 0x9FFF | J3上管约20%PWM；J2上管低、下管高 |
| 1.0 | 0 | 1250/1250 | 0x64E2 | 0x9FFF | J3上管约25%PWM；J2上管低、下管高 |
| 1.2 | +0.2 | 1500/1000 | 0x65DC | 0x9FFF | J3上管约30%PWM；J2上管低、下管高 |

0x9FFF是FPR=0、TR=8191，超过5000周期不会触发翻转，因此该逻辑臂上管保持低。启用后其互补下管可以保持高，**不等于整桥封波**。将ORI改为1会换成J2承担PWM、J3上管低/下管高；不是m正负自动决定哪侧PWM。

所以当前“Reference处于中点”或DSP注释“中性参考”并不能在这份501的N=1路径上证明实际CHB输出零电压。不能只凭DSP编译通过、软件状态进入RUNNING就认定单UNIT的电压极性及闭环调制正确。此问题应与外部排线左右归属分别核对；本次只分析，没有更改调制算法或交换线序。

## 7. NoInterlock版本的实际使能与核验方法

UNIT `main.v:91-93`把CHB/DAB外部互锁旁路固定为1；659～669行绕过最终fault/shutdown门控。CHB的SVG_EN、同步使能、复位、死区和窄脉冲过滤仍生效；OP_FO被固定0（676行），常规故障上报字也被置零（1055行），驱动原始状态字仍可另行回传。**此UNIT版本不能按“硬件FAULT/SHUTDOWN会独立封波”来使用，适用于无功率主回路台架检查。** DSP本地软件保护或命令撤销SVG_EN仍能走其控制链，但不能补回被UNIT旁路的独立硬件互锁。

推荐按以下顺序闭合证据链：

1. 核对501实际下载工程/文件；读取A1的NUM、PM、Reference、DC_Log、TAD及PM_CTL，与本报告前提比对。
2. 在功率主回路不带电的条件下，沿驱动排线或主回路图确认J2/J3各接哪个桥臂的上下管，给输入高压侧贴“物理左”标识。这一步才最终决定物理左对应逻辑L还是逻辑R。
3. 逻辑分析仪/示波器观察J2-11/15与J3-11/15，核对上/下管互补、死区、禁能行为；先以RTL内部FPL/FPR、TL/TR、SVG_PL/PR及F_HxTy信号对照，避免用正在变化的DSP Reference反推单个载波边沿。
4. 无功率台架可用已知不对称光纤字验证线序，例如单交点模式、TL=1000且FPL=1得到PWML=0x63E8，TR=2000且FPR=1得到PWMR=0xA7D0；前者应走J3、后者应走J2。不要让501自动帧与手动注入争用同一通道；这里是识别波形示例，不是修改参数后的功率运行步骤。
5. 核验/修正501单模块调制、方向/排序配置及实际物理极性，再开展有功率的闭环试验；同时换回具备完整互锁的UNIT构建。

DSP临时输入注入、AnyEyes/VOFA选择、DPRAM开关只影响控制输入/命令来源，不会改写上述UNIT四路引脚映射。本报告未进行实板测量，也未证明现场位流与所读源码完全一致。
