#include "ChbBuildConfig.h"
#include "CpuTask.h"
#include "SystemInit.h"
#include "TaskSlice.h"

/*==================================================================
 * 函数功能: 协调启动阶段后进入永久后台调度循环
 * 输入参数: 无
 * 输出参数: 所属模块状态；调用顺序和原实现保持。
 * 返回值: 初始化后进入永久循环，正常运行不返回
 * 执行说明: 系统初始化或主循环；不增加 FPGA 中断调用。
 *================================================================*/
int main(void)
{
    CpuTask_InitBootFlags();
    SystemInit_Hardware();
#if CHB_OPEN_LOOP_PULSE_TEST
    for(;;) { TaskSlice_Service(); }
#else
    SystemInit_WaitPowerOnDelay();
    SystemInit_Runtime();
    for(;;) { TaskSlice_Service(); }
#endif
}
