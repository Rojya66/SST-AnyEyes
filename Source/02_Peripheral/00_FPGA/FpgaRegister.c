#include "SstTypes.h"
/* 文件职责: FPGA 固定映射与硬件控制字缓存；从原主文件迁移，符号、类型、初值及硬件地址保持。 */
#include "FpgaRegister.h"
#include "SstControlConfig.h"
#pragma DATA_SECTION(FPGA_Interface, ".FPGA_InterfaceFile");

volatile FPGA_InterfaceBLOCK
    FPGA_Interface[15]; /* FPGA_Interface：15 个插件的 EMIFA 映射寄存器窗口，索引为原插件编号减 2。 */

XXG_DSP_CTL Port_DSP_CTL; /* Port_DSP_CTL：端口 FPGA 全关断及硬件复位控制字缓存。 */

XXG_SwitchINOUT Port_DOUT; /* Port_DOUT：端口断路器、接触器数字输出控制字缓存。 */

XXG_PM_CTL Module_PM_CTL; /* Module_PM_CTL：模块工频桥/DAB 使能、驱动复位及测试控制字缓存。 */

ushort WrongTimes_LEFT, /* WrongTimes_LEFT：原 FPGA LEFT 参考/通信核对的累计不一致次数。 */
    WrongTimes_RIGHT, /* WrongTimes_RIGHT：原 FPGA RIGHT 参考/通信核对的累计不一致次数。 */
    WrongTimes_LEFT_A, /* WrongTimes_LEFT_A：原 FPGA LEFT_A 参考/通信核对的累计不一致次数。 */
    WrongTimes_RIGHT_A; /* WrongTimes_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的累计不一致次数。 */

ushort LastWrong_LEFT, /* LastWrong_LEFT：原 FPGA LEFT 参考/通信核对的上一轮不一致值。 */
    LastWrong_RIGHT, /* LastWrong_RIGHT：原 FPGA RIGHT 参考/通信核对的上一轮不一致值。 */
    LastWrong_LEFT_A, /* LastWrong_LEFT_A：原 FPGA LEFT_A 参考/通信核对的上一轮不一致值。 */
    LastWrong_RIGHT_A; /* LastWrong_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的上一轮不一致值。 */

ushort Wrong_LEFT, /* Wrong_LEFT：原 FPGA LEFT 参考/通信核对的本轮不一致值。 */
    Wrong_RIGHT, /* Wrong_RIGHT：原 FPGA RIGHT 参考/通信核对的本轮不一致值。 */
    Wrong_LEFT_A, /* Wrong_LEFT_A：原 FPGA LEFT_A 参考/通信核对的本轮不一致值。 */
    Wrong_RIGHT_A; /* Wrong_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的本轮不一致值。 */

ushort WrongTimes_Reference_A1, /* WrongTimes_Reference_A1：原 FPGA Reference_A1 参考/通信核对的累计不一致次数。 */
    WrongTimes_Reference_A2, /* WrongTimes_Reference_A2：原 FPGA Reference_A2 参考/通信核对的累计不一致次数。 */
    WrongTimes_Reference_C1, /* WrongTimes_Reference_C1：原 FPGA Reference_C1 参考/通信核对的累计不一致次数。 */
    WrongTimes_Reference_C2; /* WrongTimes_Reference_C2：原 FPGA Reference_C2 参考/通信核对的累计不一致次数。 */

ushort LastWrong_Reference_A1, /* LastWrong_Reference_A1：原 FPGA Reference_A1 参考/通信核对的上一轮不一致值。 */
    LastWrong_Reference_A2, /* LastWrong_Reference_A2：原 FPGA Reference_A2 参考/通信核对的上一轮不一致值。 */
    LastWrong_Reference_C1, /* LastWrong_Reference_C1：原 FPGA Reference_C1 参考/通信核对的上一轮不一致值。 */
    LastWrong_Reference_C2; /* LastWrong_Reference_C2：原 FPGA Reference_C2 参考/通信核对的上一轮不一致值。 */

ushort Wrong_Reference_A1, /* Wrong_Reference_A1：原 FPGA Reference_A1 参考/通信核对的本轮不一致值。 */
    Wrong_Reference_A2, /* Wrong_Reference_A2：原 FPGA Reference_A2 参考/通信核对的本轮不一致值。 */
    Wrong_Reference_C1, /* Wrong_Reference_C1：原 FPGA Reference_C1 参考/通信核对的本轮不一致值。 */
    Wrong_Reference_C2; /* Wrong_Reference_C2：原 FPGA Reference_C2 参考/通信核对的本轮不一致值。 */

ushort
    WrongTimes_ControlReference_A, /* WrongTimes_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的累计不一致次数。 */
    WrongTimes_ControlReference_C; /* WrongTimes_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的累计不一致次数。 */

ushort
    LastWrong_ControlReference_A, /* LastWrong_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的上一轮不一致值。 */
    LastWrong_ControlReference_C; /* LastWrong_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的上一轮不一致值。 */

ushort
    Wrong_ControlReference_A, /* Wrong_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的本轮不一致值。 */
    Wrong_ControlReference_C; /* Wrong_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的本轮不一致值。 */

float DAC_BUF[4]; /* DAC_BUF：四路 DA 调试输出缓存，写出时转为 FPGA 原始计数。 */
