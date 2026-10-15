/* 文件职责: TaskSlice.h 模块公开接口。 */
#ifndef TASKSLICE_H_
#define TASKSLICE_H_
#ifdef __cplusplus
extern "C"
{
#endif
    /*==================================================================
 * 函数功能: 按固定次序执行一轮后台任务
 * 输入参数: 无
 * 输出参数: 所属模块状态；调用顺序和原实现保持。
 * 返回值: 无
 * 执行说明: 系统初始化或主循环；不增加 FPGA 中断调用。
 *================================================================*/
    void TaskSlice_Service(void);
#ifdef __cplusplus
}
#endif
#endif
