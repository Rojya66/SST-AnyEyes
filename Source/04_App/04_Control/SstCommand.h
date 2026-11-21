#ifndef SST_COMMAND_H_
#define SST_COMMAND_H_
#include "SstParamTypes.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /*==================================================================
 * 函数功能: 检查控制状态、命令和实际PWM输出均已停止
 * 输入参数: 无
 * 输出参数: 无
 * 返回值: 完全停止为1，否则为0
 * 执行说明: 后台参数访问及命令互锁，保持原参数模块的判定表达式。
 *================================================================*/
    uint16_t SstCommand_IsStopped(void);
    /*==================================================================
 * 函数功能: 在编译权限和存储互锁允许时提交串口启停命令
 * 输入参数: enable：0停止、1启动；其他数值返回范围错误
 * 输出参数: g_chb_dab_command 的run_enable、dab_enable
 * 返回值: SST_PARAM_* 参数服务结果码
 * 执行说明: 仅提交命令，PWM动作仍由控制核心执行。
 *================================================================*/
    uint16_t SstCommand_RunSerial(uint16_t enable);
    /*==================================================================
 * 函数功能: 无条件撤销CHB和DAB运行命令
 * 输入参数: 无
 * 输出参数: run_enable和dab_enable清零
 * 返回值: 无
 * 执行说明: 保留原调试急停语义，实际关断由原控制路径执行。
 *================================================================*/
    void SstCommand_EmergencyStop(void);
    /*==================================================================
 * 函数功能: 按串口权限、数值范围和停机条件写入单个运行命令字段
 * 输入参数: index：固定F0索引，0为CHB运行、1为DAB使能、3为故障复位
 *           value：0撤销、1请求；其他编码无效
 * 输出参数: 仅更新指定命令字段，不联动另一使能字段
 * 返回值: SST_PARAM_OK/RANGE/BUSY/DENIED/BAD_ADDRESS
 * 执行说明: SstParams已按原顺序处理存储忙互锁；本接口保持原F0权限及复位检查。
 *================================================================*/
    uint16_t SstCommand_WriteSerialField(uint16_t index, uint16_t value);
    /*==================================================================
 * 函数功能: 将DPRAM启动、脉冲及复位有效位赋给实际控制命令
 * 输入参数: start：块1 word2 / 00F2，bit0仅请求HVAC/CHB运行
 *           stop：块1 word3 / 00F3，bit0整机停止，优先撤销两路请求
 *           pulse_on：脉冲使能位图，bit0 为 DAB、bit1 为 CHB
 *           pulse_off：脉冲禁用位图，bit0 为 DAB、bit1 为 CHB，优先于使能
 *           reset：块1 word10 / 00FA，bit0请求故障复位，仅完全停机时接受
 * 输出参数: g_chb_dab_command.run_enable、dab_enable、fault_reset
 * 返回值: 无
 * 执行说明: 后台赋值入口。HVAC START不修改dab_enable；实际发波由原状态机决定。
 *================================================================*/
    void SstControlCommand_ApplyRequests(unsigned short start,
                               unsigned short stop,
                               unsigned short pulse_on,
                               unsigned short pulse_off,
                               unsigned short reset);
#ifdef __cplusplus
}
#endif
#endif
