#ifndef SST_BOARD_H_
#define SST_BOARD_H_
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Background-only millisecond timebase; does not install or change an ISR. */
/*==================================================================
 * 函数功能: 联合读取 Timer2 计数及 10 ms 节拍，生成单调的 1 ms 后台时间基准
 * 输入参数: 无
 * 输出参数: 无指针输出；保存上一输出值以维持单调性。
 * 返回值: 32 位毫秒节拍，允许自然回绕
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
uint32_t SstBoard_GetTick1ms(void);
#ifdef __cplusplus
}
#endif
#endif
