/* 文件职责: DpramDefaults 所属数据域的默认初始化；保持原连续语句段。 */
#ifndef DPRAMDEFAULTS_H
#define DPRAMDEFAULTS_H
#ifdef __cplusplus
extern "C"
{
#endif

    /*==================================================================
 * 函数功能: 建立原DPRAM遥调缓存的默认保护使能和给定
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
    void DpramDefaults_InitRegulation(void);

#ifdef __cplusplus
}
#endif
#endif
