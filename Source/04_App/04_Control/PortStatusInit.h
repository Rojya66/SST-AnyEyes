/* 文件职责: PortStatusInit 所属数据域的默认初始化；保持原连续语句段。 */
#ifndef PORTSTATUSINIT_H
#define PORTSTATUSINIT_H
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================
 * 函数功能: 建立原端口主模式、停机状态、故障初值及页0恢复
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void PortStatusInit_DefaultState(void);

#ifdef __cplusplus
}
#endif
#endif
