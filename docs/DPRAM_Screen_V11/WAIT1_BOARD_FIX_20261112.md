# DPRAM WAIT1 板级初始化修正

日期：2026-11-12。

## 现场现象与验证

屏幕通过 ARM 向 Modbus 字地址 `0x1FFE` 写入 2，屏幕能回读 2，DSP 能收到 DPRAM 中断，但 `DPRAM_Read()` 读取左邮箱后得到 0。

主控 FPGA 已在 `D:\Code\FPGA\Pango\MainControl` 修正为 `input DPRAM_XREADY`、`assign READY = DPRAM_XREADY`，并重新生成、烧录位流。DSP 端随后观察到：

- `0x6800001C`（CE5CFG）为 `0x7FFFFFFD`。
- `0x68000004`（AWCC）为 `0xF0400000`。
- 将 AWCC 临时改为 `0xD0400000` 后，现场确认读取恢复正常。

两种 AWCC 数值仅相差 bit29（WP1）。WP1=1 表示 WAIT1 高有效，WP1=0 表示低有效。DPRAM 的 BUSYL# 低有效，空闲时为高；原极性会把空闲误判为等待。上述现场对照确认 WAIT1 极性错误影响了本次读取，不能推断所有等待超时都必然返回 0。

## 修改位置与方式

仅修改 `Source/03_System/00_Board/SstBoard.c` 的 `EMIFASetup()`。

官方 `EMIFAWaitPinPolaritySet()` 使用按位或写入；低有效参数为 0，不能清除原有的 WP1。因此在调用原接口之前由板级代码清除 WP1：

```c
HWREG(SOC_EMIFA_0_REGS + EMIFA_AWCC) &= ~EMIFA_AWCC_WP1;
EMIFAWaitPinPolaritySet(SOC_EMIFA_0_REGS,
                       EMIFA_EMA_WAIT_PIN1,
                       EMIFA_EMA_WAIT_PIN_POLARITY_LOW);
```

后续仍通过原接口选择 CS5 使用 WAIT1，并配置最大扩展等待周期；初始化末尾增加一次 AWCC 的 volatile 读回，完成配置访问。读回不等于自动比较或故障检查。

官方 `emifa.c`、`emifa.h` 和 `hw_emifa2.h` 保持原样。WAIT0、CS2/CS5 既有时序、10 kHz 中断和通信点表均不修改。本次初始化在中断初始化之前执行，不增加实时 ISR 工作。

## 下载后的验证

1. 在停机、无脉冲条件下载新固件，复位并执行到硬件初始化之后；保持使用已修正的主控 FPGA 位流。
2. Suspend DSP，CCS 使用 32-bit Hex 查看 AWCC `0x68000004`，确认 `(AWCC & 0x20000000) == 0`，以及 `(AWCC & 0x00C00000) == 0x00400000`。保留位不用于判定配置是否正确；不要硬编码写入整个 `0xD0400000`。
3. CE5CFG `0x6800001C` 的时序本次未调整。现场原值 `0x7FFFFFFD` 仍可能保留，不应将它误认为 WAIT1 极性修复失败。
4. 屏幕向空闲字地址 `0x1B10` 分别写 `0xA55A`、`0x5AA5`，每次通过 Modbus 03 回读；CCS 在取消 L1D/L2 观察层后用 16-bit Hex 核对 DSP 地址 `0x66003620`。
5. 单次写邮箱 `0x1FFE=2`，在后台 `DPRAM_Read()` 的赋值后暂停，确认 `FLAG_NEW_L==2`。邮箱窗口不持续自动刷新；读邮箱会应答中断，但不清除存储内容。
6. 再次复位并重复上述步骤，确认无需手动修改 AWCC 即可读取。

尚需单独评估的问题：官方时序接口同样使用按位或，复位后的最大时序未被改小。本次保持已经验证能通信的时序，不与极性修复同时调整，避免改变 FPGA 访问时间和实时中断总线占用。

## 软件验证边界

- CCS 12.5 实际导入、Debug 和 Release 清理后全量编译/链接均通过，构建期间固件输入保持不变。固件输入 SHA256 为 `9bc60a04ed35324fabad890c53e525fbb3f9c4344b53f194b4663b2e9e1a4cf4`；日志和报告见 `build/ccs_validation/`。
- 已生成 `Debug/SST_DpramCompat.out` 和 `Release/SST_DpramCompat.out`。
- 使用当前板级初始化函数及原官方驱动函数执行 28 组主机寄存器模型检查：覆盖现场值、复位值、WP1 已清零和不同原时序值；修复后 WP1 均为 0，CS5 使用 WAIT1，其他寄存器与修改前初始化结果相同，重复初始化保持稳定。夹具和报告见 `build/wait1_board_fix/`，它不模拟 EMIFA 电气时序或真实等待超时。
- SHA256 核对确认官方 `emifa.c`、`emifa.h`、`hw_emifa2.h` 和 `Redef_Interrupt.c` 未改动；用户已有的 `TaskSlice.c`、launch 调试配置也保留原样。

软件编译和寄存器模型检查不能替代重新下载后的控制板验证；现场已验证的是手工清 WP1 的效果，新固件复位后自动配置的效果需按上述步骤实测。本次未重新执行其他控制算法的历史全套回归，也未重测 ISR 执行时间。
