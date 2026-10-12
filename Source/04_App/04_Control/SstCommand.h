#ifndef SST_COMMAND_H_
#define SST_COMMAND_H_
#ifdef __cplusplus
extern "C" {
#endif
/* Map the existing DPRAM words to the retained controller; no MMIO bypass. */
/*==================================================================
 * 函数功能: 按停止和关脉冲优先规则将 DPRAM 字段接入统一控制命令
 * 输入参数: start：DPRAM/维护启动位图，停止/关脉冲优先
 *           stop：DPRAM/维护停止位图，停止优先于启动
 *           pulse_on：脉冲使能位图，bit0 为 DAB、bit1 为 CHB
 *           pulse_off：脉冲禁用位图，bit0 为 DAB、bit1 为 CHB，优先于使能
 *           reset：故障复位请求位图，仅停机时接受
 * 输出参数: g_chb_dab_command 的启停、DAB 使能及故障复位字段；只提交命令。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void SstCommand_ApplyDpram(unsigned short start, unsigned short stop,
                          unsigned short pulse_on, unsigned short pulse_off,
                          unsigned short reset);
#ifdef __cplusplus
}
#endif
#endif
